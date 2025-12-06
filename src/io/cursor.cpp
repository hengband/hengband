#include "io/cursor.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "io/screen-util.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "term/term-color-types.h"
#include "view/display-map.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
    row -= panel_row_prt;
    term_gotoxy(panel_col_of(col), row);
}

/*
 * @brief 矢などの軌跡を*で表示する / print project path
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 */
void print_path(PlayerType *player_ptr, POSITION y, POSITION x)
{
    uint8_t default_color = TERM_SLATE;
    const Pos2D pos(y, x);
    if (!display_path || (project_length == -1)) {
        return;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto range = project_length != 0 ? project_length : AngbandSystem::get_instance().get_max_range();
    ProjectionPath path_g(floor, range, p_pos, p_pos, pos, PROJECT_PATH | PROJECT_THRU);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MAP);
    handle_stuff(player_ptr);
    for (const auto &pos_path : path_g) {
        const auto &grid = floor.get_grid(pos_path);
        if (panel_contains(pos_path)) {
            DisplaySymbolPair symbol_pair({ default_color, '\0' }, { default_color, '*' });
            if (grid.has_monster() && floor.m_list[grid.m_idx].ml) {
                symbol_pair = map_info(player_ptr, pos_path);
                auto &symbol_foreground = symbol_pair.symbol_foreground;
                if (!symbol_foreground.is_ascii_graphics()) {
                    symbol_foreground.color = default_color;
                } else if ((symbol_foreground.character == '.') && ((symbol_foreground.color == TERM_WHITE) || (symbol_foreground.color == TERM_L_WHITE))) {
                    symbol_foreground.color = default_color;
                } else if (symbol_foreground.color == default_color) {
                    symbol_foreground.color = TERM_WHITE;
                }
            }

            symbol_pair.symbol_foreground.color = get_monochrome_display_color(player_ptr).value_or(symbol_pair.symbol_foreground.color);
            symbol_pair.symbol_foreground.character = '*';
            term_queue_bigchar(panel_col_of(pos_path.x), pos_path.y - panel_row_prt, symbol_pair);
        }

        if (grid.is_mark() && !grid.has(TerrainCharacteristics::PROJECTION)) {
            break;
        }

        if (pos_path == pos) {
            default_color = TERM_L_DARK;
        }
    }
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(PlayerType *player_ptr, POSITION dy, POSITION dx)
{
    const auto &[wid, hgt] = get_screen_size();
    POSITION y = panel_row_min + dy * hgt / 2;
    POSITION x = panel_col_min + dx * wid / 2;

    const auto &floor = *player_ptr->current_floor_ptr;
    if (y > floor.height - hgt) {
        y = floor.height - hgt;
    }
    if (y < 0) {
        y = 0;
    }

    if (x > floor.width - wid) {
        x = floor.width - wid;
    }
    if (x < 0) {
        x = 0;
    }

    if ((y == panel_row_min) && (x == panel_col_min)) {
        return false;
    }

    panel_row_min = y;
    panel_col_min = x;
    panel_bounds_center();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief コンソール上におけるマップ表示の左上位置を返す /
 * Calculates current boundaries Called below and from "do_cmd_locate()".
 */
void panel_bounds_center(void)
{
    const auto &[wid, hgt] = get_screen_size();
    panel_row_max = panel_row_min + hgt - 1;
    panel_row_prt = panel_row_min - 1;
    panel_col_max = panel_col_min + wid - 1;
    panel_col_prt = panel_col_min - 13;
}
