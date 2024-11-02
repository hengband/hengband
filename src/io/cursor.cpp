#include "io/cursor.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-list.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/screen-util.h"
#include "player/player-status.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
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

    auto *floor_ptr = &FloorList::get_instance().get_floor(0);
    ProjectionPath path_g(player_ptr, (project_length ? project_length : AngbandSystem::get_instance().get_max_range()), player_ptr->get_position(), { y, x }, PROJECT_PATH | PROJECT_THRU);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MAP);
    handle_stuff(player_ptr);
    for (const auto &pos_path : path_g) {
        auto *g_ptr = &floor_ptr->get_grid(pos_path);
        if (panel_contains(pos_path.y, pos_path.x)) {
            DisplaySymbolPair symbol_pair({ default_color, '\0' }, { default_color, '*' });
            if (g_ptr->has_monster() && floor_ptr->m_list[g_ptr->m_idx].ml) {
                symbol_pair = map_info(player_ptr, pos_path);
                auto &symbol_foreground = symbol_pair.symbol_foreground;
                if (!is_ascii_graphics(symbol_foreground.color)) {
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

        if (g_ptr->is_mark() && !g_ptr->cave_has_flag(TerrainCharacteristics::PROJECT)) {
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

    auto *floor_ptr = &FloorList::get_instance().get_floor(0);
    if (y > floor_ptr->height - hgt) {
        y = floor_ptr->height - hgt;
    }
    if (y < 0) {
        y = 0;
    }

    if (x > floor_ptr->width - wid) {
        x = floor_ptr->width - wid;
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
