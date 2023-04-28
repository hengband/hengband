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
#include "object/item-tester-hooker.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "window/display-sub-windows.h"
#include "window/main-window-left-frame.h"
#include "window/main-window-row-column.h"
#include "window/main-window-stat-poster.h"
#include "window/main-window-util.h"
#include "world/world-turn-processor.h"
#include "world/world.h"

/*!
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @todo ここにPlayerType を追加するとz-termに影響が行くので保留
 */
void redraw_window(void)
{
    if (!w_ptr->character_dungeon) {
        return;
    }

    p_ptr->window_flags = PW_ALL;

    handle_stuff(p_ptr);
    term_redraw();
}

/*!
 * @brief 現在のマップ名を描画する / Print dungeon
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void print_dungeon(PlayerType *player_ptr)
{
    TERM_LEN width, height;
    term_get_size(&width, &height);

    c_put_str(TERM_WHITE, "             ", height + ROW_DUNGEON, COL_DUNGEON);
    concptr dungeon_name = map_name(player_ptr);
    TERM_LEN col = COL_DUNGEON + 6 - strlen(dungeon_name) / 2;
    if (col < 0) {
        col = 0;
    }

    c_put_str(TERM_L_UMBER, format("%s", dungeon_name), height + ROW_DUNGEON, col);
}

/*!
 * @brief redraw のフラグに応じた更新をまとめて行う / Handle "redraw"
 * @details 更新処理の対象はゲーム中の全描画処理
 */
void redraw_stuff(PlayerType *player_ptr)
{
    if (!player_ptr->redraw) {
        return;
    }

    if (!w_ptr->character_generated) {
        return;
    }

    if (w_ptr->character_icky_depth > 0) {
        return;
    }

    if (player_ptr->redraw & (PR_WIPE)) {
        player_ptr->redraw &= ~(PR_WIPE);
        msg_print(nullptr);
        term_clear();
    }

    if (player_ptr->redraw & (PR_MAP)) {
        player_ptr->redraw &= ~(PR_MAP);
        print_map(player_ptr);
    }

    if (player_ptr->redraw & (PR_BASIC)) {
        player_ptr->redraw &= ~(PR_BASIC);
        player_ptr->redraw &= ~(PR_MISC | PR_TITLE | PR_ABILITY_SCORE);
        player_ptr->redraw &= ~(PR_LEVEL | PR_EXP | PR_GOLD);
        player_ptr->redraw &= ~(PR_AC | PR_HP | PR_MP);
        player_ptr->redraw &= ~(PR_DEPTH | PR_HEALTH | PR_UHEALTH);
        print_frame_basic(player_ptr);
        WorldTurnProcessor(player_ptr).print_time();
        print_dungeon(player_ptr);
    }

    if (player_ptr->redraw & (PR_EQUIPPY)) {
        player_ptr->redraw &= ~(PR_EQUIPPY);
        display_player_equippy(player_ptr, ROW_EQUIPPY, COL_EQUIPPY, 0);
    }

    if (player_ptr->redraw & (PR_MISC)) {
        player_ptr->redraw &= ~(PR_MISC);
        print_field(rp_ptr->title, ROW_RACE, COL_RACE);
    }

    if (player_ptr->redraw & (PR_TITLE)) {
        player_ptr->redraw &= ~(PR_TITLE);
        print_title(player_ptr);
    }

    if (player_ptr->redraw & (PR_LEVEL)) {
        player_ptr->redraw &= ~(PR_LEVEL);
        print_level(player_ptr);
    }

    if (player_ptr->redraw & (PR_EXP)) {
        player_ptr->redraw &= ~(PR_EXP);
        print_exp(player_ptr);
    }

    if (player_ptr->redraw & (PR_ABILITY_SCORE)) {
        player_ptr->redraw &= ~(PR_ABILITY_SCORE);
        print_stat(player_ptr, A_STR);
        print_stat(player_ptr, A_INT);
        print_stat(player_ptr, A_WIS);
        print_stat(player_ptr, A_DEX);
        print_stat(player_ptr, A_CON);
        print_stat(player_ptr, A_CHR);
    }

    if (player_ptr->redraw & (PR_TIMED_EFFECT)) {
        player_ptr->redraw &= ~(PR_TIMED_EFFECT);
        print_status(player_ptr);
    }

    if (player_ptr->redraw & (PR_AC)) {
        player_ptr->redraw &= ~(PR_AC);
        print_ac(player_ptr);
    }

    if (player_ptr->redraw & (PR_HP)) {
        player_ptr->redraw &= ~(PR_HP);
        print_hp(player_ptr);
    }

    if (player_ptr->redraw & (PR_MP)) {
        player_ptr->redraw &= ~(PR_MP);
        print_sp(player_ptr);
    }

    if (player_ptr->redraw & (PR_GOLD)) {
        player_ptr->redraw &= ~(PR_GOLD);
        print_gold(player_ptr);
    }

    if (player_ptr->redraw & (PR_DEPTH)) {
        player_ptr->redraw &= ~(PR_DEPTH);
        print_depth(player_ptr);
    }

    if (player_ptr->redraw & (PR_UHEALTH)) {
        player_ptr->redraw &= ~(PR_UHEALTH);
        print_health(player_ptr, true);
    }

    if (player_ptr->redraw & (PR_HEALTH)) {
        player_ptr->redraw &= ~(PR_HEALTH);
        print_health(player_ptr, false);
    }

    if (player_ptr->redraw & (PR_EXTRA)) {
        player_ptr->redraw &= ~(PR_EXTRA);
        player_ptr->redraw &= ~(PR_CUT | PR_STUN);
        player_ptr->redraw &= ~(PR_HUNGER);
        player_ptr->redraw &= ~(PR_ACTION | PR_SPEED | PR_STUDY | PR_IMITATION | PR_TIMED_EFFECT);
        print_frame_extra(player_ptr);
    }

    if (player_ptr->redraw & (PR_CUT)) {
        player_ptr->redraw &= ~(PR_CUT);
        print_cut(player_ptr);
    }

    if (player_ptr->redraw & (PR_STUN)) {
        player_ptr->redraw &= ~(PR_STUN);
        print_stun(player_ptr);
    }

    if (player_ptr->redraw & (PR_HUNGER)) {
        player_ptr->redraw &= ~(PR_HUNGER);
        print_hunger(player_ptr);
    }

    if (player_ptr->redraw & (PR_ACTION)) {
        player_ptr->redraw &= ~(PR_ACTION);
        print_state(player_ptr);
    }

    if (player_ptr->redraw & (PR_SPEED)) {
        player_ptr->redraw &= ~(PR_SPEED);
        print_speed(player_ptr);
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::IMITATOR)) {
        if (player_ptr->redraw & (PR_IMITATION)) {
            player_ptr->redraw &= ~(PR_IMITATION);
            print_imitation(player_ptr);
        }

        return;
    }

    if (player_ptr->redraw & (PR_STUDY)) {
        player_ptr->redraw &= ~(PR_STUDY);
        print_study(player_ptr);
    }
}

/*!
 * @brief player_ptr->window のフラグに応じた更新をまとめて行う / Handle "player_ptr->window"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 更新処理の対象はサブウィンドウ全般
 */
void window_stuff(PlayerType *player_ptr)
{
    if (!player_ptr->window_flags) {
        return;
    }

    BIT_FLAGS mask = 0L;
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if (angband_terms[i] && !angband_terms[i]->never_fresh) {
            mask |= window_flag[i];
        }
    }
    BIT_FLAGS window_flags = player_ptr->window_flags & mask;

    if (window_flags & (PW_INVENTORY)) {
        player_ptr->window_flags &= ~(PW_INVENTORY);
        fix_inventory(player_ptr);
    }

    if (window_flags & (PW_EQUIPMENT)) {
        player_ptr->window_flags &= ~(PW_EQUIPMENT);
        fix_equip(player_ptr);
    }

    if (window_flags & (PW_SPELL)) {
        player_ptr->window_flags &= ~(PW_SPELL);
        fix_spell(player_ptr);
    }

    if (window_flags & (PW_PLAYER)) {
        player_ptr->window_flags &= ~(PW_PLAYER);
        fix_player(player_ptr);
    }

    // モンスターBGM対応のため、視界内モンスター表示のサブウインドウなし時も処理を行う
    if (player_ptr->window_flags & (PW_SIGHT_MONSTERS)) {
        player_ptr->window_flags &= ~(PW_SIGHT_MONSTERS);
        fix_monster_list(player_ptr);
    }

    if (window_flags & (PW_MESSAGE)) {
        player_ptr->window_flags &= ~(PW_MESSAGE);
        fix_message();
    }

    if (window_flags & (PW_OVERHEAD)) {
        player_ptr->window_flags &= ~(PW_OVERHEAD);
        fix_overhead(player_ptr);
    }

    if (window_flags & (PW_DUNGEON)) {
        player_ptr->window_flags &= ~(PW_DUNGEON);
        fix_dungeon(player_ptr);
    }

    if (window_flags & (PW_MONSTER_LORE)) {
        player_ptr->window_flags &= ~(PW_MONSTER_LORE);
        fix_monster(player_ptr);
    }

    if (window_flags & (PW_ITEM_KNOWLEDGTE)) {
        player_ptr->window_flags &= ~(PW_ITEM_KNOWLEDGTE);
        fix_object(player_ptr);
    }

    if (any_bits(window_flags, PW_FLOOR_ITEMS)) {
        reset_bits(player_ptr->window_flags, PW_FLOOR_ITEMS);
        // ウィンドウサイズ変更に対応できず。カーソル位置を取る必要がある。
        fix_floor_item_list(player_ptr, player_ptr->y, player_ptr->x);
    }

    if (any_bits(window_flags, PW_FOUND_ITEMS)) {
        reset_bits(player_ptr->window_flags, PW_FOUND_ITEMS);
        fix_found_item_list(player_ptr);
    }
}
