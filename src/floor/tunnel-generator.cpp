﻿#include "floor/tunnel-generator.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"

/*!
 * @brief build_tunnel用に通路を掘るための方向をランダムに決める / Pick a random direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @return なし
 */
static void rand_dir(POSITION *rdir, POSITION *cdir)
{
    int i = randint0(4);
    *rdir = ddy_ddd[i];
    *cdir = ddx_ddd[i];
}

/*!
 * @brief build_tunnel用に通路を掘るための方向を位置関係通りに決める / Always picks a correct direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @return なし
 */
static void correct_dir(POSITION *rdir, POSITION *cdir, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    *rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
    *cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;
    if (*rdir && *cdir) {
        if (randint0(100) < 50)
            *rdir = 0;
        else
            *cdir = 0;
    }
}

/*!
 * @brief 部屋間のトンネルを生成する / Constructs a tunnel between two points
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param row1 始点Y座標
 * @param col1 始点X座標
 * @param row2 終点Y座標
 * @param col2 終点X座標
 * @return 生成に成功したらTRUEを返す
 */
bool build_tunnel(player_type *player_ptr, dun_data_type *dd_ptr, dt_type *dt_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2)
{
    POSITION tmp_row, tmp_col;
    POSITION row_dir, col_dir;
    POSITION start_row, start_col;
    int main_loop_count = 0;
    bool door_flag = FALSE;
    grid_type *g_ptr;
    start_row = row1;
    start_col = col1;
    correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    while ((row1 != row2) || (col1 != col2)) {
        if (main_loop_count++ > 2000)
            return FALSE;

        if (randint0(100) < dt_ptr->dun_tun_chg) {
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
            if (randint0(100) < dt_ptr->dun_tun_rnd)
                rand_dir(&row_dir, &col_dir);
        }

        tmp_row = row1 + row_dir;
        tmp_col = col1 + col_dir;
        while (!in_bounds(floor_ptr, tmp_row, tmp_col)) {
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
            if (randint0(100) < dt_ptr->dun_tun_rnd)
                rand_dir(&row_dir, &col_dir);

            tmp_row = row1 + row_dir;
            tmp_col = col1 + col_dir;
        }

        g_ptr = &floor_ptr->grid_array[tmp_row][tmp_col];
        if (is_solid_grid(g_ptr))
            continue;

        if (is_outer_grid(g_ptr)) {
            POSITION y = tmp_row + row_dir;
            POSITION x = tmp_col + col_dir;
            if (is_outer_bold(floor_ptr, y, x) || is_solid_bold(floor_ptr, y, x))
                continue;

            row1 = tmp_row;
            col1 = tmp_col;
            if (dd_ptr->wall_n >= WALL_MAX)
                return FALSE;

            dd_ptr->wall[dd_ptr->wall_n].y = row1;
            dd_ptr->wall[dd_ptr->wall_n].x = col1;
            dd_ptr->wall_n++;
            for (y = row1 - 1; y <= row1 + 1; y++)
                for (x = col1 - 1; x <= col1 + 1; x++)
                    if (is_outer_bold(floor_ptr, y, x))
                        place_bold(player_ptr, y, x, GB_SOLID_NOPERM);

        } else if (g_ptr->info & (CAVE_ROOM)) {
            row1 = tmp_row;
            col1 = tmp_col;
        } else if (is_extra_grid(g_ptr) || is_inner_grid(g_ptr) || is_solid_grid(g_ptr)) {
            row1 = tmp_row;
            col1 = tmp_col;
            if (dd_ptr->tunn_n >= TUNN_MAX)
                return FALSE;

            dd_ptr->tunn[dd_ptr->tunn_n].y = row1;
            dd_ptr->tunn[dd_ptr->tunn_n].x = col1;
            dd_ptr->tunn_n++;
            door_flag = FALSE;
        } else {
            row1 = tmp_row;
            col1 = tmp_col;
            if (!door_flag) {
                if (dd_ptr->door_n >= DOOR_MAX)
                    return FALSE;

                dd_ptr->door[dd_ptr->door_n].y = row1;
                dd_ptr->door[dd_ptr->door_n].x = col1;
                dd_ptr->door_n++;
                door_flag = TRUE;
            }

            if (randint0(100) >= dt_ptr->dun_tun_con) {
                tmp_row = row1 - start_row;
                if (tmp_row < 0)
                    tmp_row = (-tmp_row);

                tmp_col = col1 - start_col;
                if (tmp_col < 0)
                    tmp_col = (-tmp_col);

                if ((tmp_row > 10) || (tmp_col > 10))
                    break;
            }
        }
    }

    return TRUE;
}

/*!
 * todo 特に詳細な処理の意味を調査すべし
 * @brief トンネル生成のための基準点を指定する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 基準点を指定するX座標の参照ポインタ、適時値が修正される。
 * @param y 基準点を指定するY座標の参照ポインタ、適時値が修正される。
 * @param affectwall (調査中)
 * @return なし
 */
static bool set_tunnel(player_type *player_ptr, dun_data_type *dd_ptr, POSITION *x, POSITION *y, bool affectwall)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[*y][*x];
    if (!in_bounds(floor_ptr, *y, *x) || is_inner_grid(g_ptr))
        return TRUE;

    if (is_extra_bold(floor_ptr, *y, *x)) {
        if (dd_ptr->tunn_n >= TUNN_MAX)
            return FALSE;

        dd_ptr->tunn[dd_ptr->tunn_n].y = *y;
        dd_ptr->tunn[dd_ptr->tunn_n].x = *x;
        dd_ptr->tunn_n++;
        return TRUE;
    }

    if (is_floor_bold(floor_ptr, *y, *x))
        return TRUE;

    if (is_outer_grid(g_ptr) && affectwall) {
        if (dd_ptr->wall_n >= WALL_MAX)
            return FALSE;

        dd_ptr->wall[dd_ptr->wall_n].y = *y;
        dd_ptr->wall[dd_ptr->wall_n].x = *x;
        dd_ptr->wall_n++;
        for (int j = *y - 1; j <= *y + 1; j++)
            for (int i = *x - 1; i <= *x + 1; i++)
                if (is_outer_bold(floor_ptr, j, i))
                    place_bold(player_ptr, j, i, GB_SOLID_NOPERM);

        floor_ptr->grid_array[*y][*x].mimic = 0;
        place_bold(player_ptr, *y, *x, GB_FLOOR);
        return TRUE;
    }

    if (is_solid_grid(g_ptr) && affectwall) {
        int i = 50;
        int dy = 0;
        int dx = 0;
        while ((i > 0) && is_solid_bold(floor_ptr, *y + dy, *x + dx)) {
            dy = randint0(3) - 1;
            dx = randint0(3) - 1;
            if (!in_bounds(floor_ptr, *y + dy, *x + dx)) {
                dx = 0;
                dy = 0;
            }

            i--;
        }

        if (i == 0) {
            place_grid(player_ptr, g_ptr, GB_OUTER);
            dx = 0;
            dy = 0;
        }

        *x = *x + dx;
        *y = *y + dy;
        return FALSE;
    }

    return TRUE;
}

/*!
 * @brief 外壁を削って「カタコンベ状」の通路を作成する / This routine creates the catacomb-like tunnels by removing extra rock.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 基準点のX座標
 * @param y 基準点のY座標
 * @return なし
 */
static void create_cata_tunnel(player_type *player_ptr, dun_data_type *dd_ptr, POSITION x, POSITION y)
{
    POSITION x1 = x - 1;
    POSITION y1 = y;
    set_tunnel(player_ptr, dd_ptr, &x1, &y1, FALSE);

    x1 = x + 1;
    y1 = y;
    set_tunnel(player_ptr, dd_ptr, &x1, &y1, FALSE);

    x1 = x;
    y1 = y - 1;
    set_tunnel(player_ptr, dd_ptr, &x1, &y1, FALSE);

    x1 = x;
    y1 = y + 1;
    set_tunnel(player_ptr, dd_ptr, &x1, &y1, FALSE);
}

/*!
 * todo 詳細用調査
 * @brief トンネル生成処理（詳細調査中）/ This routine does the bulk of the work in creating the new types of tunnels.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void short_seg_hack(
    player_type *player_ptr, dun_data_type *dd_ptr, const POSITION x1, const POSITION y1, const POSITION x2, const POSITION y2, int type, int count, bool *fail)
{
    if (!(*fail))
        return;

    int length = distance(x1, y1, x2, y2);
    count++;
    POSITION x, y;
    if ((type == 1) && (length != 0)) {
        for (int i = 0; i <= length; i++) {
            x = x1 + i * (x2 - x1) / length;
            y = y1 + i * (y2 - y1) / length;
            if (!set_tunnel(player_ptr, dd_ptr, &x, &y, TRUE)) {
                if (count > 50) {
                    *fail = FALSE;
                    return;
                }

                short_seg_hack(player_ptr, dd_ptr, x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
            }
        }

        return;
    }

    if ((type != 2) && (type != 3))
        return;

    if (x1 < x2) {
        for (int i = x1; i <= x2; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, dd_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, dd_ptr, i, y1);
        }
    } else {
        for (int i = x2; i <= x1; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, dd_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, dd_ptr, i, y1);
        }
    }

    if (y1 < y2) {
        for (int i = y1; i <= y2; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, dd_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, dd_ptr, x2, i);
        }
    } else {
        for (int i = y2; i <= y1; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, dd_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, dd_ptr, x2, i);
        }
    }
}

/*!
 * todo 詳細要調査
 * @brief 特定の壁(永久壁など)を避けながら部屋間の通路を作成する / This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
 * @return なし
 */
bool build_tunnel2(player_type *player_ptr, dun_data_type *dd_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff)
{
    POSITION x3, y3, dx, dy;
    POSITION changex, changey;
    bool retval, firstsuccede;
    grid_type *g_ptr;

    int length = distance(x1, y1, x2, y2);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (length <= cutoff) {
        retval = TRUE;
        short_seg_hack(player_ptr, dd_ptr, x1, y1, x2, y2, type, 0, &retval);
        return TRUE;
    }

    dx = (x2 - x1) / 2;
    dy = (y2 - y1) / 2;
    changex = (randint0(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;
    changey = (randint0(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;
    x3 = x1 + dx + changex;
    y3 = y1 + dy + changey;
    if (!in_bounds(floor_ptr, y3, x3)) {
        x3 = (x1 + x2) / 2;
        y3 = (y1 + y2) / 2;
    }

    g_ptr = &floor_ptr->grid_array[y3][x3];
    if (is_solid_grid(g_ptr)) {
        int i = 50;
        dy = 0;
        dx = 0;
        while ((i > 0) && is_solid_bold(floor_ptr, y3 + dy, x3 + dx)) {
            dy = randint0(3) - 1;
            dx = randint0(3) - 1;
            if (!in_bounds(floor_ptr, y3 + dy, x3 + dx)) {
                dx = 0;
                dy = 0;
            }
            i--;
        }

        if (i == 0) {
            place_bold(player_ptr, y3, x3, GB_OUTER);
            dx = 0;
            dy = 0;
        }

        y3 += dy;
        x3 += dx;
        g_ptr = &floor_ptr->grid_array[y3][x3];
    }

    if (is_floor_grid(g_ptr)) {
        if (build_tunnel2(player_ptr, dd_ptr, x1, y1, x3, y3, type, cutoff)) {
            if ((floor_ptr->grid_array[y3][x3].info & CAVE_ROOM) || (randint1(100) > 95)) {
                retval = build_tunnel2(player_ptr, dd_ptr, x3, y3, x2, y2, type, cutoff);
            } else {
                retval = FALSE;
                if (dd_ptr->door_n >= DOOR_MAX)
                    return FALSE;

                dd_ptr->door[dd_ptr->door_n].y = y3;
                dd_ptr->door[dd_ptr->door_n].x = x3;
                dd_ptr->door_n++;
            }

            firstsuccede = TRUE;
        } else {
            retval = FALSE;
            firstsuccede = FALSE;
        }
    } else {
        if (build_tunnel2(player_ptr, dd_ptr, x1, y1, x3, y3, type, cutoff)) {
            retval = build_tunnel2(player_ptr, dd_ptr, x3, y3, x2, y2, type, cutoff);
            firstsuccede = TRUE;
        } else {
            retval = FALSE;
            firstsuccede = FALSE;
        }
    }

    if (firstsuccede)
        set_tunnel(player_ptr, dd_ptr, &x3, &y3, TRUE);

    return retval;
}
