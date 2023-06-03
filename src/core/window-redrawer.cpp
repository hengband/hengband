/*!
 * @brief ウィンドウの再描画処理
 * @date 2020/06/27
 * @author Hourier
 */
#include "core/window-redrawer.h"
#include "core/stuff-handler.h"
#include "floor/floor-util.h"
#include "game-option/option-flags.h"
#include "object/item-tester-hooker.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
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
void redraw_window()
{
    if (!w_ptr->character_dungeon) {
        return;
    }

    RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
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
    const auto dungeon_name = map_name(player_ptr);
    TERM_LEN col = COL_DUNGEON + 6 - dungeon_name.length() / 2;
    if (col < 0) {
        col = 0;
    }

    c_put_str(TERM_L_UMBER, dungeon_name, height + ROW_DUNGEON, col);
}

/*!
 * @brief redraw のフラグに応じた更新をまとめて行う / Handle "redraw"
 * @details 更新処理の対象はゲーム中の全描画処理
 */
void redraw_stuff(PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!rfu.any_main()) {
        return;
    }

    if (!w_ptr->character_generated) {
        return;
    }

    if (w_ptr->character_icky_depth > 0) {
        return;
    }

    if (rfu.has(MainWindowRedrawingFlag::WIPE)) {
        rfu.reset_flag(MainWindowRedrawingFlag::WIPE);
        msg_print(nullptr);
        term_clear();
    }

    if (rfu.has(MainWindowRedrawingFlag::MAP)) {
        rfu.reset_flag(MainWindowRedrawingFlag::MAP);
        print_map(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::BASIC)) {
        static constexpr auto flags = {
            MainWindowRedrawingFlag::BASIC,
            MainWindowRedrawingFlag::TITLE,
            MainWindowRedrawingFlag::ABILITY_SCORE,
            MainWindowRedrawingFlag::LEVEL,
            MainWindowRedrawingFlag::EXP,
            MainWindowRedrawingFlag::GOLD,
            MainWindowRedrawingFlag::AC,
            MainWindowRedrawingFlag::HP,
            MainWindowRedrawingFlag::MP,
            MainWindowRedrawingFlag::DEPTH,
            MainWindowRedrawingFlag::HEALTH,
            MainWindowRedrawingFlag::UHEALTH,
        };
        rfu.reset_flags(flags);
        print_frame_basic(player_ptr);
        WorldTurnProcessor(player_ptr).print_time();
        print_dungeon(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::EQUIPPY)) {
        rfu.reset_flag(MainWindowRedrawingFlag::EQUIPPY);
        display_player_equippy(player_ptr, ROW_EQUIPPY, COL_EQUIPPY, 0);
    }

    if (rfu.has(MainWindowRedrawingFlag::TITLE)) {
        rfu.reset_flag(MainWindowRedrawingFlag::TITLE);
        print_title(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::LEVEL)) {
        rfu.reset_flag(MainWindowRedrawingFlag::LEVEL);
        print_level(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::EXP)) {
        rfu.reset_flag(MainWindowRedrawingFlag::EXP);
        print_exp(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::ABILITY_SCORE)) {
        rfu.reset_flag(MainWindowRedrawingFlag::ABILITY_SCORE);
        print_stat(player_ptr, A_STR);
        print_stat(player_ptr, A_INT);
        print_stat(player_ptr, A_WIS);
        print_stat(player_ptr, A_DEX);
        print_stat(player_ptr, A_CON);
        print_stat(player_ptr, A_CHR);
    }

    if (rfu.has(MainWindowRedrawingFlag::TIMED_EFFECT)) {
        rfu.reset_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        print_status(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::AC)) {
        rfu.reset_flag(MainWindowRedrawingFlag::AC);
        print_ac(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::HP)) {
        rfu.reset_flag(MainWindowRedrawingFlag::HP);
        print_hp(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::MP)) {
        rfu.reset_flag(MainWindowRedrawingFlag::MP);
        print_sp(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::GOLD)) {
        rfu.reset_flag(MainWindowRedrawingFlag::GOLD);
        print_gold(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::DEPTH)) {
        rfu.reset_flag(MainWindowRedrawingFlag::DEPTH);
        print_depth(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::UHEALTH)) {
        rfu.reset_flag(MainWindowRedrawingFlag::UHEALTH);
        print_health(player_ptr, true);
    }

    if (rfu.has(MainWindowRedrawingFlag::HEALTH)) {
        rfu.reset_flag(MainWindowRedrawingFlag::HEALTH);
        print_health(player_ptr, false);
    }

    if (rfu.has(MainWindowRedrawingFlag::EXTRA)) {
        static constexpr auto flags = {
            MainWindowRedrawingFlag::EXTRA,
            MainWindowRedrawingFlag::CUT,
            MainWindowRedrawingFlag::STUN,
            MainWindowRedrawingFlag::HUNGER,
            MainWindowRedrawingFlag::ACTION,
            MainWindowRedrawingFlag::SPEED,
            MainWindowRedrawingFlag::STUDY,
            MainWindowRedrawingFlag::IMITATION,
            MainWindowRedrawingFlag::TIMED_EFFECT,
        };
        rfu.reset_flags(flags);
        print_frame_extra(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::CUT)) {
        rfu.reset_flag(MainWindowRedrawingFlag::CUT);
        print_cut(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::STUN)) {
        rfu.reset_flag(MainWindowRedrawingFlag::STUN);
        print_stun(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::HUNGER)) {
        rfu.reset_flag(MainWindowRedrawingFlag::HUNGER);
        print_hunger(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::ACTION)) {
        rfu.reset_flag(MainWindowRedrawingFlag::ACTION);
        print_state(player_ptr);
    }

    if (rfu.has(MainWindowRedrawingFlag::SPEED)) {
        rfu.reset_flag(MainWindowRedrawingFlag::SPEED);
        print_speed(player_ptr);
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::IMITATOR)) {
        if (rfu.has(MainWindowRedrawingFlag::IMITATION)) {
            rfu.reset_flag(MainWindowRedrawingFlag::IMITATION);
            print_imitation(player_ptr);
        }

        return;
    }

    if (rfu.has(MainWindowRedrawingFlag::STUDY)) {
        rfu.reset_flag(MainWindowRedrawingFlag::STUDY);
        print_study(player_ptr);
    }
}

/*!
 * @brief SubWindowRedrawingFlag のフラグに応じた更新をまとめて行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 更新処理の対象はサブウィンドウ全て
 */
void window_stuff(PlayerType *player_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (!rfu.any_sub()) {
        return;
    }

    EnumClassFlagGroup<SubWindowRedrawingFlag> target_flags{};
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if ((angband_terms[i] == nullptr) || angband_terms[i]->never_fresh) {
            continue;
        }

        target_flags.set(g_window_flags[i]);
    }

    const auto &window_flags = rfu.get_sub_intersection(target_flags);
    if (window_flags.has(SubWindowRedrawingFlag::INVENTORY)) {
        rfu.reset_flag(SubWindowRedrawingFlag::INVENTORY);
        fix_inventory(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::EQUIPMENT)) {
        rfu.reset_flag(SubWindowRedrawingFlag::EQUIPMENT);
        fix_equip(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::SPELL)) {
        rfu.reset_flag(SubWindowRedrawingFlag::SPELL);
        fix_spell(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::PLAYER)) {
        rfu.reset_flag(SubWindowRedrawingFlag::PLAYER);
        fix_player(player_ptr);
    }

    // モンスターBGM対応のため、視界内モンスター表示のサブウインドウなし時も処理を行う
    if (rfu.has(SubWindowRedrawingFlag::SIGHT_MONSTERS)) {
        rfu.reset_flag(SubWindowRedrawingFlag::SIGHT_MONSTERS);
        fix_monster_list(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::MESSAGE)) {
        rfu.reset_flag(SubWindowRedrawingFlag::MESSAGE);
        fix_message();
    }

    if (window_flags.has(SubWindowRedrawingFlag::OVERHEAD)) {
        rfu.reset_flag(SubWindowRedrawingFlag::OVERHEAD);
        fix_overhead(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::DUNGEON)) {
        rfu.reset_flag(SubWindowRedrawingFlag::DUNGEON);
        fix_dungeon(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::MONSTER_LORE)) {
        rfu.reset_flag(SubWindowRedrawingFlag::MONSTER_LORE);
        fix_monster(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::ITEM_KNOWLEDGTE)) {
        rfu.reset_flag(SubWindowRedrawingFlag::ITEM_KNOWLEDGTE);
        fix_object(player_ptr);
    }

    if (window_flags.has(SubWindowRedrawingFlag::FLOOR_ITEMS)) {
        rfu.reset_flag(SubWindowRedrawingFlag::FLOOR_ITEMS);
        // ウィンドウサイズ変更に対応できず。カーソル位置を取る必要がある。
        fix_floor_item_list(player_ptr, player_ptr->y, player_ptr->x);
    }

    if (window_flags.has(SubWindowRedrawingFlag::FOUND_ITEMS)) {
        rfu.reset_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
        fix_found_item_list(player_ptr);
    }
}
