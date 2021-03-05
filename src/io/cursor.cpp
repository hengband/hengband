﻿#include "io/cursor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/screen-util.h"
#include "system/floor-type-definition.h"
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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @return なし
 */
void print_path(player_type *player_ptr, POSITION y, POSITION x)
{
    u16b path_g[512];
    byte default_color = TERM_SLATE;

    if (!display_path || (project_length == -1))
        return;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int path_n = projection_path(
        player_ptr, path_g, (project_length ? project_length : get_max_range(player_ptr)), player_ptr->y, player_ptr->x, y, x, PROJECT_PATH | PROJECT_THRU);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    for (int i = 0; i < path_n; i++) {
        POSITION ny = get_grid_y(path_g[i]);
        POSITION nx = get_grid_x(path_g[i]);
        grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
        if (panel_contains(ny, nx)) {
            TERM_COLOR a = default_color;
            SYMBOL_CODE c;

            TERM_COLOR ta = default_color;
            SYMBOL_CODE tc = '*';

            if (g_ptr->m_idx && floor_ptr->m_list[g_ptr->m_idx].ml) {
                map_info(player_ptr, ny, nx, &a, &c, &ta, &tc);

                if (!is_ascii_graphics(a))
                    a = default_color;
                else if (c == '.' && (a == TERM_WHITE || a == TERM_L_WHITE))
                    a = default_color;
                else if (a == default_color)
                    a = TERM_WHITE;
            }

            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            c = '*';
            term_queue_bigchar(panel_col_of(nx), ny - panel_row_prt, a, c, ta, tc);
        }

        if ((g_ptr->info & CAVE_MARK) && !cave_has_flag_grid(g_ptr, FF_PROJECT))
            break;

        if (nx == x && ny == y)
            default_color = TERM_L_DARK;
    }
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);

    POSITION y = panel_row_min + dy * hgt / 2;
    POSITION x = panel_col_min + dx * wid / 2;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (y > floor_ptr->height - hgt)
        y = floor_ptr->height - hgt;
    if (y < 0)
        y = 0;

    if (x > floor_ptr->width - wid)
        x = floor_ptr->width - wid;
    if (x < 0)
        x = 0;

    if ((y == panel_row_min) && (x == panel_col_min))
        return FALSE;

    panel_row_min = y;
    panel_col_min = x;
    panel_bounds_center();

    player_ptr->update |= (PU_MONSTERS);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    return TRUE;
}

/*!
 * @brief コンソール上におけるマップ表示の左上位置を返す /
 * Calculates current boundaries Called below and from "do_cmd_locate()".
 * @return なし
 */
void panel_bounds_center(void)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    panel_row_max = panel_row_min + hgt - 1;
    panel_row_prt = panel_row_min - 1;
    panel_col_max = panel_col_min + wid - 1;
    panel_col_prt = panel_col_min - 13;
}
