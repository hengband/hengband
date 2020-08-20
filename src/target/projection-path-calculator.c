#include "target/projection-path-calculator.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"

/*
 * @brief Convert a "location" (Y, X) into a "grid" (G)
 * @param y Y座標
 * @param x X座標
 * return 経路座標
 */
static u16b location_to_grid(POSITION y, POSITION x) { return 256 * y + x; }

/*!
 * @brief 始点から終点への直線経路を返す /
 * Determine the path taken by a projection.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param gp 経路座標リストを返す参照ポインタ
 * @param range 距離
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @param flg フラグID
 * @return リストの長さ
 */
int projection_path(player_type *player_ptr, u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg)
{
    if ((x1 == x2) && (y1 == y2))
        return 0;

    POSITION y, x;
    POSITION ay, ax;
    POSITION sy, sx;
    int frac;
    int m;

    if (y2 < y1) {
        ay = (y1 - y2);
        sy = -1;
    } else {
        ay = (y2 - y1);
        sy = 1;
    }

    if (x2 < x1) {
        ax = (x1 - x2);
        sx = -1;
    } else {
        ax = (x2 - x1);
        sx = 1;
    }

    int half = (ay * ax);
    int full = half << 1;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int n = 0;
    int k = 0;

    /* Vertical */
    if (ay > ax) {
        m = ax * ax * 2;
        y = y1 + sy;
        x = x1;
        frac = m;
        if (frac > half) {
            x += sx;
            frac -= full;
            k++;
        }

        while (TRUE) {
            gp[n++] = location_to_grid(y, x);
            if ((n + (k >> 1)) >= range)
                break;

            if (!(flg & PROJECT_THRU)) {
                if ((x == x2) && (y == y2))
                    break;
            }

            if (flg & PROJECT_DISI) {
                if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x))
                    break;
            } else if (flg & PROJECT_LOS) {
                if ((n > 0) && !cave_los_bold(floor_ptr, y, x))
                    break;
            } else if (!(flg & PROJECT_PATH)) {
                if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT))
                    break;
            }

            if (flg & PROJECT_STOP) {
                if ((n > 0) && (player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
                    break;
            }

            if (!in_bounds(floor_ptr, y, x))
                break;

            if (m) {
                frac += m;
                if (frac > half) {
                    x += sx;
                    frac -= full;
                    k++;
                }
            }

            y += sy;
        }

        return n;
    }

    /* Horizontal */
    if (ax > ay) {
        m = ay * ay * 2;
        y = y1;
        x = x1 + sx;
        frac = m;
        if (frac > half) {
            y += sy;
            frac -= full;
            k++;
        }

        while (TRUE) {
            gp[n++] = location_to_grid(y, x);
            if ((n + (k >> 1)) >= range)
                break;

            if (!(flg & (PROJECT_THRU))) {
                if ((x == x2) && (y == y2))
                    break;
            }

            if (flg & (PROJECT_DISI)) {
                if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x))
                    break;
            } else if (flg & (PROJECT_LOS)) {
                if ((n > 0) && !cave_los_bold(floor_ptr, y, x))
                    break;
            } else if (!(flg & (PROJECT_PATH))) {
                if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT))
                    break;
            }

            if (flg & (PROJECT_STOP)) {
                if ((n > 0) && (player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
                    break;
            }

            if (!in_bounds(floor_ptr, y, x))
                break;

            if (m) {
                frac += m;
                if (frac > half) {
                    y += sy;
                    frac -= full;
                    k++;
                }
            }

            x += sx;
        }

        return n;
    }

    y = y1 + sy;
    x = x1 + sx;

    while (TRUE) {
        gp[n++] = location_to_grid(y, x);
        if ((n + (n >> 1)) >= range)
            break;

        if (!(flg & PROJECT_THRU)) {
            if ((x == x2) && (y == y2))
                break;
        }

        if (flg & PROJECT_DISI) {
            if ((n > 0) && cave_stop_disintegration(floor_ptr, y, x))
                break;
        } else if (flg & PROJECT_LOS) {
            if ((n > 0) && !cave_los_bold(floor_ptr, y, x))
                break;
        } else if (!(flg & PROJECT_PATH)) {
            if ((n > 0) && !cave_have_flag_bold(floor_ptr, y, x, FF_PROJECT))
                break;
        }

        if (flg & PROJECT_STOP) {
            if ((n > 0) && (player_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].m_idx != 0))
                break;
        }

        if (!in_bounds(floor_ptr, y, x))
            break;

        y += sy;
        x += sx;
    }

    return n;
}
