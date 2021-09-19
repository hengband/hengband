#include "world/world-turn-processor.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-save.h"
#include "core/disturbance.h"
#include "core/magic-effects-timeout-reducer.h"
#include "dungeon/dungeon.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "hpmp/hp-mp-processor.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-curse.h"
#include "inventory/recharge-processor.h"
#include "io/write-diary.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "mutation/mutation-processor.h"
#include "object/lite-processor.h"
#include "perception/simple-perception.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-row-column.h"
#include "world/world-movement-processor.h"
#include "world/world.h"

WorldTurnProcessor::WorldTurnProcessor(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 10ゲームターンが進行する毎にゲーム世界全体の処理を行う。
 * / Handle certain things once every 10 game turns
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void WorldTurnProcessor::process_world()
{
    const int32_t a_day = TURNS_PER_TICK * TOWN_DAWN;
    int32_t prev_turn_in_today = ((w_ptr->game_turn - TURNS_PER_TICK) % a_day + a_day / 4) % a_day;
    int prev_min = (1440 * prev_turn_in_today / a_day) % 60;

    int dummy_day;
    extract_day_hour_min(this->player_ptr, &dummy_day, &this->hour, &this->min);
    update_dungeon_feeling(this->player_ptr);
    process_downward();
    process_monster_arena();
    if (w_ptr->game_turn % TURNS_PER_TICK) {
        return;
    }

    decide_auto_save();
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (floor_ptr->monster_noise && !ignore_unview) {
        msg_print(_("何かが聞こえた。", "You hear noise."));
    }

    process_change_daytime_night();
    process_world_monsters();
    if (!this->hour && !this->min) {
        if (this->min != prev_min) {
            exe_write_diary(this->player_ptr, DIARY_DIALY, 0, nullptr);
            determine_daily_bounty(this->player_ptr, false);
        }
    }

    ring_nightmare_bell(prev_min);
    starve_player(this->player_ptr);
    process_player_hp_mp(this->player_ptr);
    reduce_magic_effects_timeout(this->player_ptr);
    reduce_lite_life(this->player_ptr);
    process_world_aux_mutation(this->player_ptr);
    execute_cursed_items_effect(this->player_ptr);
    recharge_magic_items(this->player_ptr);
    sense_inventory1(this->player_ptr);
    sense_inventory2(this->player_ptr);
    execute_recall(this->player_ptr);
    execute_floor_reset(this->player_ptr);
}

/*!
 * @brief ゲーム時刻を表示する /
 * Print time
 */
void WorldTurnProcessor::print_time()
{
    int day;
    c_put_str(TERM_WHITE, "             ", ROW_DAY, COL_DAY);
    extract_day_hour_min(this->player_ptr, &day, &this->hour, &this->min);
    if (day < 1000)
        c_put_str(TERM_WHITE, format(_("%2d日目", "Day%3d"), day), ROW_DAY, COL_DAY);
    else
        c_put_str(TERM_WHITE, _("***日目", "Day***"), ROW_DAY, COL_DAY);

    c_put_str(TERM_WHITE, format("%2d:%02d", this->hour, this->min), ROW_DAY, COL_DAY + 7);
}

void WorldTurnProcessor::process_downward()
{
    /* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
    if (!ironman_downward || (this->player_ptr->dungeon_idx == DUNGEON_ANGBAND) || (this->player_ptr->dungeon_idx == 0)) {
        return;
    }

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    floor_ptr->dun_level = 0;
    this->player_ptr->dungeon_idx = 0;
    prepare_change_floor_mode(this->player_ptr, CFM_FIRST_FLOOR | CFM_RAND_PLACE);
    floor_ptr->inside_arena = false;
    this->player_ptr->wild_mode = false;
    this->player_ptr->leaving = true;
}

void WorldTurnProcessor::process_monster_arena()
{
    if (!this->player_ptr->phase_out || this->player_ptr->leaving) {
        return;
    }

    auto win_m_idx = 0;
    auto number_mon = 0;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor_ptr->width; ++x) {
        for (auto y = 0; y < floor_ptr->height; y++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != this->player_ptr->riding)) {
                number_mon++;
                win_m_idx = g_ptr->m_idx;
            }
        }
    }

    if (number_mon == 0) {
        msg_print(_("相打ちに終わりました。", "Nothing survived."));
        msg_print(nullptr);
        this->player_ptr->energy_need = 0;
        update_gambling_monsters(this->player_ptr);
        return;
    }

    if (number_mon == 1) {
        process_monster_arena_winner(win_m_idx);
        return;
    }

    process_monster_arena_draw();
}

void WorldTurnProcessor::process_monster_arena_winner(int win_m_idx)
{
    GAME_TEXT m_name[MAX_NLEN];
    auto *wm_ptr = &this->player_ptr->current_floor_ptr->m_list[win_m_idx];
    monster_desc(this->player_ptr, m_name, wm_ptr, 0);
    msg_format(_("%sが勝利した！", "%s won!"), m_name);
    msg_print(nullptr);

    if (win_m_idx == (sel_monster + 1)) {
        msg_print(_("おめでとうございます。", "Congratulations."));
        msg_format(_("%d＄を受け取った。", "You received %d gold."), battle_odds);
        this->player_ptr->au += battle_odds;
    } else {
        msg_print(_("残念でした。", "You lost gold."));
    }

    msg_print(nullptr);
    this->player_ptr->energy_need = 0;
    update_gambling_monsters(this->player_ptr);
}

void WorldTurnProcessor::process_monster_arena_draw()
{
    auto turn = this->player_ptr->current_floor_ptr->generated_turn;
    if (w_ptr->game_turn - turn != 150 * TURNS_PER_TICK) {
        return;
    }

    msg_print(_("申し訳ありませんが、この勝負は引き分けとさせていただきます。", "Sorry, but this battle ended in a draw."));
    this->player_ptr->au += kakekin;
    msg_print(nullptr);
    this->player_ptr->energy_need = 0;
    update_gambling_monsters(this->player_ptr);
}

void WorldTurnProcessor::decide_auto_save()
{
    if (autosave_freq == 0) {
        return;
    }

    auto should_save = autosave_t;
    should_save &= !this->player_ptr->phase_out;
    should_save &= w_ptr->game_turn % ((int32_t)autosave_freq * TURNS_PER_TICK) == 0;
    if (should_save) {
        do_cmd_save_game(this->player_ptr, true);
    }
}

void WorldTurnProcessor::process_change_daytime_night()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (!floor_ptr->dun_level && !floor_ptr->inside_quest && !this->player_ptr->phase_out && !floor_ptr->inside_arena) {
        if (!(w_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2))) {
            auto dawn = w_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN) == 0;
            if (dawn) {
                day_break(this->player_ptr);
            } else {
                night_falls(this->player_ptr);
            }
        }

        return;
    }

    auto is_in_dungeon = vanilla_town;
    is_in_dungeon |= lite_town && (floor_ptr->inside_quest == 0) && !this->player_ptr->phase_out && !floor_ptr->inside_arena;
    is_in_dungeon &= floor_ptr->dun_level != 0;
    if (!is_in_dungeon) {
        return;
    }

    if ((w_ptr->game_turn % (TURNS_PER_TICK * STORE_TICKS)) != 0) {
        return;
    }

    shuffle_shopkeeper();
}

void WorldTurnProcessor::process_world_monsters()
{
    decide_alloc_monster();
    if (!(w_ptr->game_turn % (TURNS_PER_TICK * 10)) && !this->player_ptr->phase_out) {
        regenerate_monsters(this->player_ptr);
    }

    if (!(w_ptr->game_turn % (TURNS_PER_TICK * 3))) {
        regenerate_captured_monsters(this->player_ptr);
    }

    if (this->player_ptr->leaving) {
        return;
    }

    for (auto i = 0; i < MAX_MTIMED; i++) {
        if (this->player_ptr->current_floor_ptr->mproc_max[i] > 0) {
            process_monsters_mtimed(this->player_ptr, i);
        }
    }
}

void WorldTurnProcessor::shuffle_shopkeeper()
{
    if (!one_in_(STORE_SHUFFLE)) {
        return;
    }

    int n;
    do {
        n = randint0(MAX_STORES);
        if ((n == STORE_HOME) || (n == STORE_MUSEUM)) {
            break;
        }
    } while (true);

    for (auto i = 1; i < max_f_idx; i++) {
        auto *f_ptr = &f_info[i];
        if (f_ptr->name.empty() || f_ptr->flags.has_not(FF::STORE))
            continue;

        if (f_ptr->subtype != n) {
            continue;
        }

        if (cheat_xtra) {
            msg_format(_("%sの店主をシャッフルします。", "Shuffle a Shopkeeper of %s."), f_ptr->name.c_str());
        }

        store_shuffle(this->player_ptr, n);
        break;
    }
}

void WorldTurnProcessor::decide_alloc_monster()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto should_alloc = one_in_(d_info[this->player_ptr->dungeon_idx].max_m_alloc_chance);
    should_alloc &= !floor_ptr->inside_arena;
    should_alloc &= floor_ptr->inside_quest == 0;
    should_alloc &= !this->player_ptr->phase_out;
    if (should_alloc) {
        (void)alloc_monster(this->player_ptr, MAX_SIGHT + 5, 0, summon_specific);
    }
}

/*
 * Nightmare mode activates the TY_CURSE at midnight
 * Require exact minute -- Don't activate multiple times in a minute
 */
void WorldTurnProcessor::ring_nightmare_bell(int prev_min)
{
    if (!ironman_nightmare || (this->min == prev_min)) {
        return;
    }

    if ((this->hour == 23) && !(this->min % 15)) {
        disturb(this->player_ptr, false, true);
        switch (this->min / 15) {
        case 0:
            msg_print(_("遠くで不気味な鐘の音が鳴った。", "You hear a distant bell toll ominously."));
            break;

        case 1:
            msg_print(_("遠くで鐘が二回鳴った。", "A distant bell sounds twice."));
            break;

        case 2:
            msg_print(_("遠くで鐘が三回鳴った。", "A distant bell sounds three times."));
            break;

        case 3:
            msg_print(_("遠くで鐘が四回鳴った。", "A distant bell tolls four times."));
            break;
        }
    }

    if ((this->hour > 0) || (this->min > 0)) {
        return;
    }

    disturb(this->player_ptr, true, true);
    msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));
    if (this->player_ptr->wild_mode) {
        this->player_ptr->oldpy = randint1(MAX_HGT - 2);
        this->player_ptr->oldpx = randint1(MAX_WID - 2);
        change_wild_mode(this->player_ptr, true);
        PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    }

    this->player_ptr->invoking_midnight_curse = true;
}
