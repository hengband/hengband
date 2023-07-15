/*!
 * @brief 画面描画のユーティリティ
 * @date 2018/09/25
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "io/screen-util.h"
#include "core/player-processor.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-town.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "monster/monster-update.h"
#include "player-info/mimic-info-table.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-map.h"
#include "window/main-window-row-column.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief コンソールのリサイズに合わせてマップを再描画する /
 * Map resizing whenever the main term changes size
 * @todo ここにPlayerType を追加するとz-termに影響が行くので保留
 */
void resize_map()
{
    if (!w_ptr->character_dungeon) {
        return;
    }

    panel_row_max = 0;
    panel_col_max = 0;
    panel_row_min = p_ptr->current_floor_ptr->height;
    panel_col_min = p_ptr->current_floor_ptr->width;
    verify_panel(p_ptr);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    rfu.set_flags(flags_mwrf);
    handle_stuff(p_ptr);
    term_redraw();

    if (can_save) {
        move_cursor_relative(p_ptr->y, p_ptr->x);
    }

    term_fresh();
}

/*!
 * @brief 現在のコンソール表示の縦横を返す。 /
 * Get term size and calculate screen size
 * @param wid_p コンソールの表示幅文字数を返す
 * @param hgt_p コンソールの表示行数を返す
 */
void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p)
{
    term_get_size(wid_p, hgt_p);
    *hgt_p -= ROW_MAP + 2;
    *wid_p -= COL_MAP + 2;
    if (use_bigtile) {
        *wid_p /= 2;
    }
}

/*
 * Determines if a map location is currently "on screen" -RAK-
 * Note that "panel_contains(Y,X)" always implies "in_bounds2(Y,X)".
 */
bool panel_contains(POSITION y, POSITION x)
{
    return (y >= panel_row_min) && (y <= panel_row_max) && (x >= panel_col_min) && (x <= panel_col_max);
}
