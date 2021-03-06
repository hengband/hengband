#include "world/world-turn-processor.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-save.h"
#include "core/disturbance.h"
#include "core/hp-mp-processor.h"
#include "core/hp-mp-regenerator.h"
#include "core/magic-effects-timeout-reducer.h"
#include "dungeon/dungeon.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
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
#include "player/digestion-processor.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-row-column.h"
#include "world/world-movement-processor.h"
#include "world/world.h"

/*!
 * @brief 10ゲームターンが進行する毎にゲーム世界全体の処理を行う。
 * / Handle certain things once every 10 game turns
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void process_world(player_type *player_ptr)
{
    const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
    s32b prev_turn_in_today = ((current_world_ptr->game_turn - TURNS_PER_TICK) % A_DAY + A_DAY / 4) % A_DAY;
    int prev_min = (1440 * prev_turn_in_today / A_DAY) % 60;

    int day, hour, min;
    extract_day_hour_min(player_ptr, &day, &hour, &min);
    update_dungeon_feeling(player_ptr);

    /* 帰還無しモード時のレベルテレポバグ対策 / Fix for level teleport bugs on ironman_downward.*/
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (ironman_downward && (player_ptr->dungeon_idx != DUNGEON_ANGBAND && player_ptr->dungeon_idx != 0)) {
        floor_ptr->dun_level = 0;
        player_ptr->dungeon_idx = 0;
        prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR | CFM_RAND_PLACE);
        floor_ptr->inside_arena = FALSE;
        player_ptr->wild_mode = FALSE;
        player_ptr->leaving = TRUE;
    }

    if (player_ptr->phase_out && !player_ptr->leaving) {
        int win_m_idx = 0;
        int number_mon = 0;
        for (int i2 = 0; i2 < floor_ptr->width; ++i2) {
            for (int j2 = 0; j2 < floor_ptr->height; j2++) {
                grid_type *g_ptr = &floor_ptr->grid_array[j2][i2];
                if ((g_ptr->m_idx > 0) && (g_ptr->m_idx != player_ptr->riding)) {
                    number_mon++;
                    win_m_idx = g_ptr->m_idx;
                }
            }
        }

        if (number_mon == 0) {
            msg_print(_("相打ちに終わりました。", "Nothing survived."));
            msg_print(NULL);
            player_ptr->energy_need = 0;
            update_gambling_monsters(player_ptr);
        } else if ((number_mon - 1) == 0) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_type *wm_ptr;
            wm_ptr = &floor_ptr->m_list[win_m_idx];
            monster_desc(player_ptr, m_name, wm_ptr, 0);
            msg_format(_("%sが勝利した！", "%s won!"), m_name);
            msg_print(NULL);

            if (win_m_idx == (sel_monster + 1)) {
                msg_print(_("おめでとうございます。", "Congratulations."));
                msg_format(_("%d＄を受け取った。", "You received %d gold."), battle_odds);
                player_ptr->au += battle_odds;
            } else {
                msg_print(_("残念でした。", "You lost gold."));
            }

            msg_print(NULL);
            player_ptr->energy_need = 0;
            update_gambling_monsters(player_ptr);
        } else if (current_world_ptr->game_turn - floor_ptr->generated_turn == 150 * TURNS_PER_TICK) {
            msg_print(_("申し訳ありませんが、この勝負は引き分けとさせていただきます。", "Sorry, but this battle ended in a draw."));
            player_ptr->au += kakekin;
            msg_print(NULL);
            player_ptr->energy_need = 0;
            update_gambling_monsters(player_ptr);
        }
    }

    if (current_world_ptr->game_turn % TURNS_PER_TICK)
        return;

    if (autosave_t && autosave_freq && !player_ptr->phase_out) {
        if (!(current_world_ptr->game_turn % ((s32b)autosave_freq * TURNS_PER_TICK)))
            do_cmd_save_game(player_ptr, TRUE);
    }

    if (floor_ptr->monster_noise && !ignore_unview) {
        msg_print(_("何かが聞こえた。", "You hear noise."));
    }

    if (!floor_ptr->dun_level && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena) {
        if (!(current_world_ptr->game_turn % ((TURNS_PER_TICK * TOWN_DAWN) / 2))) {
            bool dawn = (!(current_world_ptr->game_turn % (TURNS_PER_TICK * TOWN_DAWN)));
            if (dawn)
                day_break(player_ptr);
            else
                night_falls(player_ptr);
        }
    } else if ((vanilla_town || (lite_town && !floor_ptr->inside_quest && !player_ptr->phase_out && !floor_ptr->inside_arena)) && floor_ptr->dun_level) {
        if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * STORE_TICKS))) {
            if (one_in_(STORE_SHUFFLE)) {
                int n;
                do {
                    n = randint0(MAX_STORES);
                } while ((n == STORE_HOME) || (n == STORE_MUSEUM));

                for (FEAT_IDX i = 1; i < max_f_idx; i++) {
                    feature_type *f_ptr = &f_info[i];
                    if (!f_ptr->name)
                        continue;
                    if (!has_flag(f_ptr->flags, FF_STORE))
                        continue;

                    if (f_ptr->subtype == n) {
                        if (cheat_xtra)
                            msg_format(_("%sの店主をシャッフルします。", "Shuffle a Shopkeeper of %s."), f_name + f_ptr->name);

                        store_shuffle(player_ptr, n);
                        break;
                    }
                }
            }
        }
    }

    if (one_in_(d_info[player_ptr->dungeon_idx].max_m_alloc_chance) && !floor_ptr->inside_arena && !floor_ptr->inside_quest && !player_ptr->phase_out) {
        (void)alloc_monster(player_ptr, MAX_SIGHT + 5, 0, summon_specific);
    }

    if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 10)) && !player_ptr->phase_out)
        regenerate_monsters(player_ptr);
    if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 3)))
        regenerate_captured_monsters(player_ptr);

    if (!player_ptr->leaving) {
        for (int i = 0; i < MAX_MTIMED; i++) {
            if (floor_ptr->mproc_max[i] > 0)
                process_monsters_mtimed(player_ptr, i);
        }
    }

    if (!hour && !min) {
        if (min != prev_min) {
            exe_write_diary(player_ptr, DIARY_DIALY, 0, NULL);
            determine_daily_bounty(player_ptr, FALSE);
        }
    }

    /*
     * Nightmare mode activates the TY_CURSE at midnight
     * Require exact minute -- Don't activate multiple times in a minute
     */
    if (ironman_nightmare && (min != prev_min)) {
        if ((hour == 23) && !(min % 15)) {
            disturb(player_ptr, FALSE, TRUE);
            switch (min / 15) {
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

        if (!hour && !min) {
            disturb(player_ptr, TRUE, TRUE);
            msg_print(_("遠くで鐘が何回も鳴り、死んだような静けさの中へ消えていった。", "A distant bell tolls many times, fading into an deathly silence."));
            if (player_ptr->wild_mode) {
                player_ptr->oldpy = randint1(MAX_HGT - 2);
                player_ptr->oldpx = randint1(MAX_WID - 2);
                change_wild_mode(player_ptr, TRUE);
                take_turn(player_ptr, 100);
            }

            player_ptr->invoking_midnight_curse = TRUE;
        }
    }

    starve_player(player_ptr);
    process_player_hp_mp(player_ptr);
    reduce_magic_effects_timeout(player_ptr);
    reduce_lite_life(player_ptr);
    process_world_aux_mutation(player_ptr);
    execute_cursed_items_effect(player_ptr);
    recharge_magic_items(player_ptr);
    sense_inventory1(player_ptr);
    sense_inventory2(player_ptr);
    execute_recall(player_ptr);
    execute_floor_reset(player_ptr);
}

/*!
 * @brief ゲーム時刻を表示する /
 * Print time
 * @return なし
 */
void print_time(player_type *player_ptr)
{
    int day, hour, min;
    c_put_str(TERM_WHITE, "             ", ROW_DAY, COL_DAY);
    extract_day_hour_min(player_ptr, &day, &hour, &min);
    if (day < 1000)
        c_put_str(TERM_WHITE, format(_("%2d日目", "Day%3d"), day), ROW_DAY, COL_DAY);
    else
        c_put_str(TERM_WHITE, _("***日目", "Day***"), ROW_DAY, COL_DAY);

    c_put_str(TERM_WHITE, format("%2d:%02d", hour, min), ROW_DAY, COL_DAY + 7);
}
