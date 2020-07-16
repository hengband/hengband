/*!
 * @brief ウィンドウの再描画処理
 * @date 2020/06/27
 * @author Hourier
 */
#include "core/window-redrawer.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "player/player-race.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "window/display-sub-windows.h"
#include "window/display-sub-window-spells.h"
#include "window/main-window-left-frame.h"
#include "window/main-window-row-column.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world-turn-processor.h"
#include "world/world.h"

/*!
 * todo ここにplayer_type を追加するとz-termに影響が行くので保留
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @return なし
 */
void redraw_window(void)
{
    if (!current_world_ptr->character_dungeon)
        return;

    p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

    handle_stuff(p_ptr);
    term_redraw();
}

/*!
 * @brief 現在のマップ名を描画する / Print dungeon
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void print_dungeon(player_type *creature_ptr)
{
    c_put_str(TERM_WHITE, "             ", ROW_DUNGEON, COL_DUNGEON);
    concptr dungeon_name = map_name(creature_ptr);
    TERM_LEN col = COL_DUNGEON + 6 - strlen(dungeon_name) / 2;
    if (col < 0)
        col = 0;

    c_put_str(TERM_L_UMBER, format("%s", dungeon_name), ROW_DUNGEON, col);
}

/*!
 * @brief redraw のフラグに応じた更新をまとめて行う / Handle "redraw"
 * @return なし
 * @details 更新処理の対象はゲーム中の全描画処理
 */
void redraw_stuff(player_type *creature_ptr)
{
    if (!creature_ptr->redraw)
        return;

    if (!current_world_ptr->character_generated)
        return;

    if (current_world_ptr->character_icky)
        return;

    if (creature_ptr->redraw & (PR_WIPE)) {
        creature_ptr->redraw &= ~(PR_WIPE);
        msg_print(NULL);
        term_clear();
    }

    if (creature_ptr->redraw & (PR_MAP)) {
        creature_ptr->redraw &= ~(PR_MAP);
        print_map(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_BASIC)) {
        creature_ptr->redraw &= ~(PR_BASIC);
        creature_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_STATS);
        creature_ptr->redraw &= ~(PR_LEV | PR_EXP | PR_GOLD);
        creature_ptr->redraw &= ~(PR_ARMOR | PR_HP | PR_MANA);
        creature_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH | PR_UHEALTH);
        print_frame_basic(creature_ptr);
        print_time(creature_ptr);
        print_dungeon(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_EQUIPPY)) {
        creature_ptr->redraw &= ~(PR_EQUIPPY);
        display_player_equippy(creature_ptr, ROW_EQUIPPY, COL_EQUIPPY, 0);
    }

    if (creature_ptr->redraw & (PR_MISC)) {
        creature_ptr->redraw &= ~(PR_MISC);
        print_field(rp_ptr->title, ROW_RACE, COL_RACE);
    }

    if (creature_ptr->redraw & (PR_TITLE)) {
        creature_ptr->redraw &= ~(PR_TITLE);
        print_title(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_LEV)) {
        creature_ptr->redraw &= ~(PR_LEV);
        print_level(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_EXP)) {
        creature_ptr->redraw &= ~(PR_EXP);
        print_exp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STATS)) {
        creature_ptr->redraw &= ~(PR_STATS);
        print_stat(creature_ptr, A_STR);
        print_stat(creature_ptr, A_INT);
        print_stat(creature_ptr, A_WIS);
        print_stat(creature_ptr, A_DEX);
        print_stat(creature_ptr, A_CON);
        print_stat(creature_ptr, A_CHR);
    }

    if (creature_ptr->redraw & (PR_STATUS)) {
        creature_ptr->redraw &= ~(PR_STATUS);
        print_status(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_ARMOR)) {
        creature_ptr->redraw &= ~(PR_ARMOR);
        print_ac(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HP)) {
        creature_ptr->redraw &= ~(PR_HP);
        print_hp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_MANA)) {
        creature_ptr->redraw &= ~(PR_MANA);
        print_sp(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_GOLD)) {
        creature_ptr->redraw &= ~(PR_GOLD);
        print_gold(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_DEPTH)) {
        creature_ptr->redraw &= ~(PR_DEPTH);
        print_depth(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HEALTH)) {
        creature_ptr->redraw &= ~(PR_HEALTH);
        health_redraw(creature_ptr, FALSE);
    }

    if (creature_ptr->redraw & (PR_UHEALTH)) {
        creature_ptr->redraw &= ~(PR_UHEALTH);
        health_redraw(creature_ptr, TRUE);
    }

    if (creature_ptr->redraw & (PR_EXTRA)) {
        creature_ptr->redraw &= ~(PR_EXTRA);
        creature_ptr->redraw &= ~(PR_CUT | PR_STUN);
        creature_ptr->redraw &= ~(PR_HUNGER);
        creature_ptr->redraw &= ~(PR_STATE | PR_SPEED | PR_STUDY | PR_IMITATION | PR_STATUS);
        print_frame_extra(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_CUT)) {
        creature_ptr->redraw &= ~(PR_CUT);
        print_cut(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STUN)) {
        creature_ptr->redraw &= ~(PR_STUN);
        print_stun(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_HUNGER)) {
        creature_ptr->redraw &= ~(PR_HUNGER);
        print_hunger(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_STATE)) {
        creature_ptr->redraw &= ~(PR_STATE);
        print_state(creature_ptr);
    }

    if (creature_ptr->redraw & (PR_SPEED)) {
        creature_ptr->redraw &= ~(PR_SPEED);
        print_speed(creature_ptr);
    }

    if (creature_ptr->pclass == CLASS_IMITATOR) {
        if (creature_ptr->redraw & (PR_IMITATION)) {
            creature_ptr->redraw &= ~(PR_IMITATION);
            print_imitation(creature_ptr);
        }

        return;
    }

    if (creature_ptr->redraw & (PR_STUDY)) {
        creature_ptr->redraw &= ~(PR_STUDY);
        print_study(creature_ptr);
    }
}

/*!
 * @brief player_ptr->window のフラグに応じた更新をまとめて行う / Handle "player_ptr->window"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details 更新処理の対象はサブウィンドウ全般
 */
void window_stuff(player_type *player_ptr)
{
    if (!player_ptr->window)
        return;

    BIT_FLAGS mask = 0L;
    for (int j = 0; j < 8; j++) {
        if (angband_term[j])
            mask |= window_flag[j];
    }

    player_ptr->window &= mask;

    if (!player_ptr->window)
        return;

    if (player_ptr->window & (PW_INVEN)) {
        player_ptr->window &= ~(PW_INVEN);
        fix_inventory(player_ptr, 0); // TODO:2.2.2 まともなtval参照手段を確保
    }

    if (player_ptr->window & (PW_EQUIP)) {
        player_ptr->window &= ~(PW_EQUIP);
        fix_equip(player_ptr, 0); // TODO:2.2.2 まともなtval参照手段を確保
    }

    if (player_ptr->window & (PW_SPELL)) {
        player_ptr->window &= ~(PW_SPELL);
        fix_spell(player_ptr);
    }

    if (player_ptr->window & (PW_PLAYER)) {
        player_ptr->window &= ~(PW_PLAYER);
        fix_player(player_ptr);
    }

    if (player_ptr->window & (PW_MONSTER_LIST)) {
        player_ptr->window &= ~(PW_MONSTER_LIST);
        fix_monster_list(player_ptr);
    }

    if (player_ptr->window & (PW_MESSAGE)) {
        player_ptr->window &= ~(PW_MESSAGE);
        fix_message();
    }

    if (player_ptr->window & (PW_OVERHEAD)) {
        player_ptr->window &= ~(PW_OVERHEAD);
        fix_overhead(player_ptr);
    }

    if (player_ptr->window & (PW_DUNGEON)) {
        player_ptr->window &= ~(PW_DUNGEON);
        fix_dungeon(player_ptr);
    }

    if (player_ptr->window & (PW_MONSTER)) {
        player_ptr->window &= ~(PW_MONSTER);
        fix_monster(player_ptr);
    }

    if (player_ptr->window & (PW_OBJECT)) {
        player_ptr->window &= ~(PW_OBJECT);
        fix_object(player_ptr);
    }
}
