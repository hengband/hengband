#include "system/angband.h"
#include "market/arena.h"
#include "market/arena-info-table.h"
#include "market/building-util.h"
#include "player/player-effects.h"
#include "core/stuff-handler.h"
#include "core/show-file.h"
#include "monster/monsterrace-hook.h"
#include "world/world.h"
#include "dungeon/dungeon.h"

/*!
 * @brief 優勝時のメッセージを表示し、賞金を与える
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return まだ優勝していないか、挑戦者モンスターとの戦いではFALSE
 */
static bool process_ostensible_arena_victory(player_type *player_ptr)
{
    if (player_ptr->arena_number != MAX_ARENA_MONS)
        return FALSE;

    clear_bldg(5, 19);
    prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
    prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
    prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

    prt("", 10, 0);
    prt("", 11, 0);
    player_ptr->au += 1000000L;
    msg_print(_("スペースキーで続行", "Press the space bar to continue"));
    msg_print(NULL);
    player_ptr->arena_number++;
    return TRUE;
}

/*!
 * @brief はぐれメタルとの対戦
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return まだパワー・ワイアーム以下を倒していないならFALSE、倒していたらTRUE
 */
static bool battle_metal_babble(player_type *player_ptr)
{
    if (player_ptr->arena_number <= MAX_ARENA_MONS)
        return FALSE;

    if (player_ptr->arena_number >= MAX_ARENA_MONS + 2) {
        msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。",
            "You enter the arena briefly and bask in your glory."));
        msg_print(NULL);
        return TRUE;
    }

    msg_print(_("君のために最強の挑戦者を用意しておいた。", "The strongest challenger is waiting for you."));
    msg_print(NULL);
    if (!get_check(_("挑戦するかね？", "Do you fight? "))) {
        msg_print(_("残念だ。", "We are disappointed."));
        return TRUE;
    }

    msg_print(_("死ぬがよい。", "Die, maggots."));
    msg_print(NULL);

    player_ptr->exit_bldg = FALSE;
    reset_tim_flags(player_ptr);

    /* Save the surface floor as saved floor */
    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

    player_ptr->current_floor_ptr->inside_arena = TRUE;
    player_ptr->leaving = TRUE;
    player_ptr->leave_bldg = TRUE;
    return TRUE;
}

static void go_to_arena(player_type *player_ptr)
{
    if (process_ostensible_arena_victory(player_ptr))
        return;

    if (battle_metal_babble(player_ptr))
        return;

    if (player_ptr->riding && (player_ptr->pclass != CLASS_BEASTMASTER) && (player_ptr->pclass != CLASS_CAVALRY)) {
        msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。",
            "You don't have permission to enter with pet."));
        msg_print(NULL);
        return;
    }

    player_ptr->exit_bldg = FALSE;
    reset_tim_flags(player_ptr);
    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS);

    player_ptr->current_floor_ptr->inside_arena = TRUE;
    player_ptr->leaving = TRUE;
    player_ptr->leave_bldg = TRUE;
}

static void see_arena_poster(player_type *player_ptr)
{
    if (player_ptr->arena_number == MAX_ARENA_MONS) {
        msg_print(_("あなたは勝利者だ。 アリーナでのセレモニーに参加しなさい。",
            "You are victorious. Enter the arena for the ceremony."));
        return;
    }

    if (player_ptr->arena_number > MAX_ARENA_MONS) {
        msg_print(_("あなたはすべての敵に勝利した。", "You have won against all foes."));
        return;
    }

    monster_race *r_ptr;
    r_ptr = &r_info[arena_info[player_ptr->arena_number].r_idx];
    concptr name = (r_name + r_ptr->name);
    msg_format(_("%s に挑戦するものはいないか？", "Do I hear any challenges against: %s"), name);

    player_ptr->monster_race_idx = arena_info[player_ptr->arena_number].r_idx;
    player_ptr->window |= (PW_MONSTER);
    handle_stuff(player_ptr);
}

/*!
 * @brief 闘技場に入るコマンドの処理 / arena commands
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param cmd 闘技場処理のID
 * @return なし
 */
void arena_comm(player_type *player_ptr, int cmd)
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

        /* Peruse the arena help file */
        (void)show_file(player_ptr, TRUE, _("arena_j.txt", "arena.txt"), NULL, 0, 0);
        screen_load();
        break;
    }
}

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void update_gambling_monsters(player_type *player_ptr)
{
    int total, i;
    int max_dl = 0;
    int mon_level;
    int power[4];
    bool tekitou;

    for (i = 0; i < current_world_ptr->max_d_idx; i++) {
        if (max_dl < max_dlv[i])
            max_dl = max_dlv[i];
    }

    mon_level = randint1(MIN(max_dl, 122)) + 5;
    if (randint0(100) < 60) {
        i = randint1(MIN(max_dl, 122)) + 5;
        mon_level = MAX(i, mon_level);
    }

    if (randint0(100) < 30) {
        i = randint1(MIN(max_dl, 122)) + 5;
        mon_level = MAX(i, mon_level);
    }

    while (TRUE) {
        total = 0;
        tekitou = FALSE;
        for (i = 0; i < 4; i++) {
            MONRACE_IDX r_idx;
            int j;
            while (TRUE) {
                get_mon_num_prep(player_ptr, monster_can_entry_arena, NULL);
                r_idx = get_mon_num(player_ptr, mon_level, GMN_ARENA);
                if (!r_idx)
                    continue;

                if ((r_info[r_idx].flags1 & RF1_UNIQUE) || (r_info[r_idx].flags7 & RF7_UNIQUE2)) {
                    if ((r_info[r_idx].level + 10) > mon_level)
                        continue;
                }

                for (j = 0; j < i; j++)
                    if (r_idx == battle_mon[j])
                        break;
                if (j < i)
                    continue;

                break;
            }
            battle_mon[i] = r_idx;
            if (r_info[r_idx].level < 45)
                tekitou = TRUE;
        }

        for (i = 0; i < 4; i++) {
            monster_race *r_ptr = &r_info[battle_mon[i]];
            int num_taisei = count_bits(r_ptr->flagsr & (RFR_IM_ACID | RFR_IM_ELEC | RFR_IM_FIRE | RFR_IM_COLD | RFR_IM_POIS));

            if (r_ptr->flags1 & RF1_FORCE_MAXHP)
                power[i] = r_ptr->hdice * r_ptr->hside * 2;
            else
                power[i] = r_ptr->hdice * (r_ptr->hside + 1);
            power[i] = power[i] * (100 + r_ptr->level) / 100;
            if (r_ptr->speed > 110)
                power[i] = power[i] * (r_ptr->speed * 2 - 110) / 100;
            if (r_ptr->speed < 110)
                power[i] = power[i] * (r_ptr->speed - 20) / 100;
            if (num_taisei > 2)
                power[i] = power[i] * (num_taisei * 2 + 5) / 10;
            else if (r_ptr->a_ability_flags2 & RF6_INVULNER)
                power[i] = power[i] * 4 / 3;
            else if (r_ptr->a_ability_flags2 & RF6_HEAL)
                power[i] = power[i] * 4 / 3;
            else if (r_ptr->a_ability_flags1 & RF5_DRAIN_MANA)
                power[i] = power[i] * 11 / 10;
            if (r_ptr->flags1 & RF1_RAND_25)
                power[i] = power[i] * 9 / 10;
            if (r_ptr->flags1 & RF1_RAND_50)
                power[i] = power[i] * 9 / 10;
            if (r_ptr->flagsr & RFR_RES_ALL)
                power[i] *= 100000;
            if (r_ptr->arena_ratio)
                power[i] = power[i] * r_ptr->arena_ratio / 100;
            total += power[i];
        }

        for (i = 0; i < 4; i++) {
            if (power[i] <= 0)
                break;
            power[i] = total * 60 / power[i];
            if (tekitou && ((power[i] < 160) || power[i] > 1500))
                break;
            if ((power[i] < 160) && randint0(20))
                break;
            if (power[i] < 101)
                power[i] = 100 + randint1(5);
            mon_odds[i] = power[i];
        }

        if (i == 4)
            break;
    }
}
