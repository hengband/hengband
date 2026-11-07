#include "io/cursor.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "io/screen-util.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/deferred-update-requests.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "term/term-color-types.h"
#include "view/display-map.h"
#include "view/map-tile-layout.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
    term_gotoxy(panel_col_of(col), panel_row_of(row));
}

/*!
 * @brief print_path() で描いた軌跡を消す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param path_grids print_path() で*を描いたグリッド。描き直した後に空にする
 * @details 地図全体ではなく、*を描いたグリッドだけを描き直す。
 */
void erase_path(PlayerType *player_ptr, std::vector<Pos2D> &path_grids)
{
    for (const auto &pos : path_grids) {
        erase_overlay_spot(player_ptr, pos);
    }

    path_grids.clear();
}

/*!
 * @brief 矢などの軌跡を*で表示する / print project path
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 目標地点の座標
 * @param path_grids 前回この関数で*を描いたグリッド。今回*を描いたグリッドに置き換える
 * @details 前回の軌跡は、地図全体ではなく描いたグリッドだけを描き直して消す。
 * 画面を動かしたときは地図全体が描き直されているので、前回のグリッドを描き直しても表示は変わらない。
 */
void print_path(PlayerType *player_ptr, const Pos2D &pos, std::vector<Pos2D> &path_grids)
{
    erase_path(player_ptr, path_grids);
    if (!display_path || (project_length == -1)) {
        return;
    }

    uint8_t default_color = TERM_SLATE;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    const auto range = project_length != 0 ? project_length : AngbandSystem::get_instance().get_max_range();
    ProjectionPath path_g(floor, range, p_pos, p_pos, pos, PROJECT_PATH | PROJECT_THRU);

    // 積まれているメイン画面の描き直しの要求を、射線を描く前に済ませておく
    // (後で地図全体が描き直されると、描いた射線が消えてしまうため)
    redraw_stuff(player_ptr);
    const auto monochrome_color = get_monochrome_display_color(player_ptr);
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

            symbol_pair.symbol_foreground.color = monochrome_color.value_or(symbol_pair.symbol_foreground.color);
            symbol_pair.symbol_foreground.character = '*';
            original_tiles::queue_map_tile(panel_col_of(pos_path.x), panel_row_of(pos_path.y), symbol_pair);
            path_grids.push_back(pos_path);
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
    DeferredUpdateRequests::get_instance().request_monster_update_and_map_redrawing();
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
