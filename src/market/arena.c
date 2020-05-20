#include "system/angband.h"
#include "market/arena.h"
#include "market/arena-info-table.h"
#include "market/building-util.h"
#include "player/player-effects.h"
#include "core/stuff-handler.h"
#include "core/show-file.h"

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
