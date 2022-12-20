#include "market/arena.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-mode-changer.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "market/arena-info-table.h"
#include "market/building-actions-table.h"
#include "market/building-util.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player-base/player-class.h"
#include "status/buff-setter.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <numeric>

/*!
 * @brief 優勝時のメッセージを表示し、賞金を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return まだ優勝していないか、挑戦者モンスターとの戦いではFALSE
 */
static bool process_ostensible_arena_victory(PlayerType *player_ptr)
{
    if (player_ptr->arena_number != MAX_ARENA_MONS) {
        return false;
    }

    clear_bldg(5, 19);
    prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
    prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
    prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

    prt("", 10, 0);
    prt("", 11, 0);
    player_ptr->au += 1000000L;
    msg_print(_("スペースキーで続行", "Press the space bar to continue"));
    msg_print(nullptr);
    player_ptr->arena_number++;
    return true;
}

/*!
 * @brief はぐれメタルとの対戦
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return まだパワー・ワイアーム以下を倒していないならFALSE、倒していたらTRUE
 */
static bool battle_metal_babble(PlayerType *player_ptr)
{
    if (player_ptr->arena_number <= MAX_ARENA_MONS) {
        return false;
    }

    if (player_ptr->arena_number >= MAX_ARENA_MONS + 2) {
        msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。", "You enter the arena briefly and bask in your glory."));
        msg_print(nullptr);
        return true;
    }

    msg_print(_("君のために最強の挑戦者を用意しておいた。", "The strongest challenger is waiting for you."));
    msg_print(nullptr);
    if (!get_check(_("挑戦するかね？", "Do you fight? "))) {
        msg_print(_("残念だ。", "We are disappointed."));
        return true;
    }

    msg_print(_("死ぬがよい。", "Die, maggots."));
    msg_print(nullptr);

    player_ptr->exit_bldg = false;
    reset_tim_flags(player_ptr);

    /* Save the surface floor as saved floor */
    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    player_ptr->leave_bldg = true;
    return true;
}

static void go_to_arena(PlayerType *player_ptr)
{
    if (process_ostensible_arena_victory(player_ptr)) {
        return;
    }

    if (battle_metal_babble(player_ptr)) {
        return;
    }

    if (player_ptr->riding && !PlayerClass(player_ptr).is_tamer()) {
        msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。", "You don't have permission to enter with pet."));
        msg_print(nullptr);
        return;
    }

    player_ptr->exit_bldg = false;
    reset_tim_flags(player_ptr);
    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    player_ptr->leave_bldg = true;
}

static void see_arena_poster(PlayerType *player_ptr)
{
    if (player_ptr->arena_number == MAX_ARENA_MONS) {
        msg_print(_("あなたは勝利者だ。 アリーナでのセレモニーに参加しなさい。", "You are victorious. Enter the arena for the ceremony."));
        return;
    }

    if (player_ptr->arena_number > MAX_ARENA_MONS) {
        msg_print(_("あなたはすべての敵に勝利した。", "You have won against all foes."));
        return;
    }

    MonsterRaceInfo *r_ptr;
    r_ptr = &monraces_info[arena_info[player_ptr->arena_number].r_idx];
    concptr name = r_ptr->name.data();
    msg_format(_("%s に挑戦するものはいないか？", "Do I hear any challenges against: %s"), name);

    player_ptr->monster_race_idx = arena_info[player_ptr->arena_number].r_idx;
    player_ptr->window_flags |= (PW_MONSTER);
    handle_stuff(player_ptr);
}

/*!
 * @brief 闘技場に入るコマンドの処理 / on_defeat_arena_monster commands
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cmd 闘技場処理のID
 */
void arena_comm(PlayerType *player_ptr, int cmd)
{
    switch (cmd) {
    case BACT_ARENA:
        go_to_arena(player_ptr);
        return;
    case BACT_POSTER:
        see_arena_poster(player_ptr);
        return;
    case BACT_ARENA_RULES:
        screen_save();

        /* Peruse the on_defeat_arena_monster help file */
        (void)show_file(player_ptr, true, _("arena_j.txt", "arena.txt"), nullptr, 0, 0);
        screen_load();
        break;
    }
}

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void update_gambling_monsters(PlayerType *player_ptr)
{
    int total, i;
    int max_dl = 0;
    int mon_level;
    int power[4];
    bool tekitou;

    for (const auto &d_ref : dungeons_info) {
        if (max_dl < max_dlv[d_ref.idx]) {
            max_dl = max_dlv[d_ref.idx];
        }
    }

    mon_level = randint1(std::min(max_dl, 122)) + 5;
    if (randint0(100) < 60) {
        i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    if (randint0(100) < 30) {
        i = randint1(std::min(max_dl, 122)) + 5;
        mon_level = std::max(i, mon_level);
    }

    while (true) {
        total = 0;
        tekitou = false;
        for (i = 0; i < 4; i++) {
            MonsterRaceId r_idx;
            int j;
            while (true) {
                get_mon_num_prep(player_ptr, monster_can_entry_arena, nullptr);
                r_idx = get_mon_num(player_ptr, 0, mon_level, GMN_ARENA);
                if (!MonsterRace(r_idx).is_valid()) {
                    continue;
                }

                if (monraces_info[r_idx].kind_flags.has(MonsterKindType::UNIQUE) || (monraces_info[r_idx].flags7 & RF7_UNIQUE2)) {
                    if ((monraces_info[r_idx].level + 10) > mon_level) {
                        continue;
                    }
                }

                for (j = 0; j < i; j++) {
                    if (r_idx == battle_mon_list[j]) {
                        break;
                    }
                }
                if (j < i) {
                    continue;
                }

                break;
            }
            battle_mon_list[i] = r_idx;
            if (monraces_info[r_idx].level < 45) {
                tekitou = true;
            }
        }

        std::transform(std::begin(battle_mon_list), std::end(battle_mon_list), std::begin(power),
            [](MonsterRace r_idx) { return MonsterRace(r_idx).calc_power(); });
        total += std::reduce(std::begin(power), std::end(power));

        for (i = 0; i < 4; i++) {
            if (power[i] <= 0) {
                break;
            }
            power[i] = total * 60 / power[i];
            if (tekitou && ((power[i] < 160) || power[i] > 1500)) {
                break;
            }
            if ((power[i] < 160) && randint0(20)) {
                break;
            }
            if (power[i] < 101) {
                power[i] = 100 + randint1(5);
            }
            mon_odds[i] = power[i];
        }

        if (i == 4) {
            break;
        }
    }
}

/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
bool monster_arena_comm(PlayerType *player_ptr)
{
    PRICE maxbet;
    PRICE wager;
    char out_val[MAX_MONSTER_NAME];
    concptr p;

    if ((w_ptr->game_turn - w_ptr->arena_start_turn) > TURNS_PER_TICK * 250) {
        update_gambling_monsters(player_ptr);
        w_ptr->arena_start_turn = w_ptr->game_turn;
    }

    screen_save();

    /* No money */
    if (player_ptr->au <= 1) {
        msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    clear_bldg(4, 10);

    prt(_("モンスター                                                     倍率", "Monsters                                                       Odds"), 4, 4);
    for (int i = 0; i < 4; i++) {
        auto *r_ptr = &monraces_info[battle_mon_list[i]];
        std::string name;
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            name = _(r_ptr->name, "Fake ");
            name.append(_("もどき", r_ptr->name));
        } else {
            name = r_ptr->name;
            name.append(_("      ", ""));
        }
        prt(format(_("%d) %-58s  %4ld.%02ld倍", "%d) %-58s  %4ld.%02ld"), i + 1, name.data(), (long int)mon_odds[i] / 100, (long int)mon_odds[i] % 100), 5 + i, 1);
    }

    prt(_("どれに賭けますか:", "Which monster: "), 0, 0);
    while (true) {
        int i = inkey();

        if (i == ESCAPE) {
            screen_load();
            return false;
        }

        if (i >= '1' && i <= '4') {
            sel_monster = i - '1';
            battle_odds = mon_odds[sel_monster];
            break;
        }

        else {
            bell();
        }
    }

    clear_bldg(4, 4);
    for (int i = 0; i < 4; i++) {
        if (i != sel_monster) {
            clear_bldg(i + 5, i + 5);
        }
    }

    maxbet = player_ptr->lev * 200;

    /* We can't bet more than we have */
    maxbet = std::min(maxbet, player_ptr->au);

    /*
     * Get the wager
     * Use get_string() because we may need more than
     * the int16_t value returned by get_quantity().
     */
    out_val[0] = '\0';
    if (!get_string(format(_("賭け金 (1-%ld)？", "Your wager (1-%ld) ? "), (long int)maxbet), out_val, 32)) {
        screen_load();
        return false;
    }

    for (p = out_val; *p == ' '; p++) {
        ;
    }

    wager = atol(p);
    if (wager > player_ptr->au) {
        msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));

        msg_print(nullptr);
        screen_load();
        return false;
    } else if (wager > maxbet) {
        msg_format(_("%ldゴールドだけ受けよう。残りは取っときな。", "I'll take %ld gold of that. Keep the rest."), (long int)maxbet);

        wager = maxbet;
    } else if (wager < 1) {
        msg_print(_("ＯＫ、１ゴールドでいこう。", "Ok, we'll start with 1 gold."));
        wager = 1;
    }

    msg_print(nullptr);
    battle_odds = std::max(wager + 1, wager * battle_odds / 100);
    kakekin = wager;
    player_ptr->au -= wager;
    reset_tim_flags(player_ptr);

    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

    player_ptr->phase_out = true;
    player_ptr->leaving = true;
    player_ptr->leave_bldg = true;

    screen_load();
    return true;
}
