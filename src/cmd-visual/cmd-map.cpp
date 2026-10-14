#include "cmd-visual/cmd-map.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "io/input-key-acceptor.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "view/display-map.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <string_view>

/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.
 */
void do_cmd_view_map(PlayerType *player_ptr)
{
    screen_save();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    term_fresh();
    term_clear();
    display_autopick.clear();

    int cy, cx;
    display_map(player_ptr, &cy, &cx);
    if (autopick_list.empty() || AngbandWorld::get_instance().is_wild_mode()) {
        const auto &[wid, hgt] = term_get_size();
        constexpr auto msg = _("何かキーを押すとゲームに戻ります", "Hit any key to continue");
        const auto center_x = (wid - std::string_view(msg).length()) / 2;
        put_str(msg, hgt - 1, center_x);
        move_cursor(cy, cx);
        inkey(true);
        screen_load();
        return;
    }

    display_autopick.set(AutopickMethod::ITEM_DISPLAY);
    while (true) {
        const auto &[wid, hgt] = term_get_size();
        int row_message = hgt - 1;
        put_str(_("何かキーを押してください('M':拾う 'N':放置 'D':M+N 'K':壊すアイテムを表示)",
                    " Hit M, N(for ~), K(for !), or D(same as M+N) to display auto-picker items."),
            row_message, 1);
        move_cursor(cy, cx);
        int i = inkey(true);
        EnumClassFlagGroup<AutopickMethod> flags;
        if ('M' == i) {
            flags.set({ AutopickMethod::AUTOPICK, AutopickMethod::QUERY_AUTOPICK });
        } else if ('N' == i) {
            flags.set(AutopickMethod::NOT_AUTOPICK);
        } else if ('K' == i) {
            flags.set(AutopickMethod::AUTODESTROY);
        } else if ('D' == i) {
            flags.set({ AutopickMethod::AUTOPICK, AutopickMethod::QUERY_AUTOPICK, AutopickMethod::NOT_AUTOPICK });
        } else {
            break;
        }

        term_fresh();
        if (display_autopick.has_all_of(flags)) {
            display_autopick.reset(flags);
        } else {
            display_autopick.set(flags);
        }

        display_map(player_ptr, &cy, &cx);
    }

    display_autopick.clear();

    // 表示中に自分の周囲のサブウィンドウが描き直されていたら、自動拾いの対象だけを強調した表示のまま残るので描き直す。
    // ダンジョン全体図は、表示中に求め直した結果を使わず全グリッドを求め直すので、変わったものとして描き直される
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::DUNGEON);
    screen_load();
}
