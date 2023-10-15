/*!
 * @brief 魔法による距離やエリアの計算
 * @date 2014/07/10
 * @author Ben Harrison, James E. Wilson, Robert A. Koeneke, deskull and Hourier
 */

#include "spell/range-calc.h"
#include "effect/attribute-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"

/*
 * Find the distance from (x, y) to a line.
 */
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    POSITION py = y1 - y;
    POSITION px = x1 - x;
    POSITION ny = x2 - x1;
    POSITION nx = y1 - y2;
    POSITION pd = distance(y1, x1, y, x);
    POSITION nd = distance(y1, x1, y2, x2);

    if (pd > nd) {
        return distance(y, x, y2, x2);
    }

    nd = ((nd) ? ((py * ny + px * nx) / nd) : 0);
    return nd >= 0 ? nd : 0 - nd;
}

/*
 *
 * Modified version of los() for calculation of disintegration balls.
 * Disintegration effects are stopped by permanent walls.
 */
bool in_disintegration_range(FloorType *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    POSITION delta_y = y2 - y1;
    POSITION delta_x = x2 - x1;
    POSITION absolute_y = std::abs(delta_y);
    POSITION absolute_x = std::abs(delta_x);
    if ((absolute_x < 2) && (absolute_y < 2)) {
        return true;
    }

    POSITION scanner_y;
    if (!delta_x) {
        /* South -- check for walls */
        if (delta_y > 0) {
            for (scanner_y = y1 + 1; scanner_y < y2; scanner_y++) {
                if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) {
                    return false;
                }
            }
        }

        /* North -- check for walls */
        else {
            for (scanner_y = y1 - 1; scanner_y > y2; scanner_y--) {
                if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) {
                    return false;
                }
            }
        }

        return true;
    }

    /* Directly East/West */
    POSITION scanner_x;
    if (!delta_y) {
        /* East -- check for walls */
        if (delta_x > 0) {
            for (scanner_x = x1 + 1; scanner_x < x2; scanner_x++) {
                if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) {
                    return false;
                }
            }
        }

        /* West -- check for walls */
        else {
            for (scanner_x = x1 - 1; scanner_x > x2; scanner_x--) {
                if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) {
                    return false;
                }
            }
        }

        return true;
    }

    POSITION sign_x = (delta_x < 0) ? -1 : 1;
    POSITION sign_y = (delta_y < 0) ? -1 : 1;
    if (absolute_x == 1) {
        if (absolute_y == 2) {
            if (!cave_stop_disintegration(floor_ptr, y1 + sign_y, x1)) {
                return true;
            }
        }
    } else if (absolute_y == 1) {
        if (absolute_x == 2) {
            if (!cave_stop_disintegration(floor_ptr, y1, x1 + sign_x)) {
                return true;
            }
        }
    }

    POSITION scale_factor_2 = (absolute_x * absolute_y);
    POSITION scale_factor_1 = scale_factor_2 << 1;
    POSITION fraction_y;
    POSITION m; /* Slope, or 1/Slope, of LOS */
    if (absolute_x >= absolute_y) {
        fraction_y = absolute_y * absolute_y;
        m = fraction_y << 1;
        scanner_x = x1 + sign_x;
        if (fraction_y == scale_factor_2) {
            scanner_y = y1 + sign_y;
            fraction_y -= scale_factor_1;
        } else {
            scanner_y = y1;
        }

        /* Note (below) the case (qy == f2), where */
        /* the LOS exactly meets the corner of a tile. */
        while (x2 - scanner_x) {
            if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) {
                return false;
            }

            fraction_y += m;

            if (fraction_y < scale_factor_2) {
                scanner_x += sign_x;
            } else if (fraction_y > scale_factor_2) {
                scanner_y += sign_y;
                if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) {
                    return false;
                }
                fraction_y -= scale_factor_1;
                scanner_x += sign_x;
            } else {
                scanner_y += sign_y;
                fraction_y -= scale_factor_1;
                scanner_x += sign_x;
            }
        }

        return true;
    }

    POSITION fraction_x = absolute_x * absolute_x;
    m = fraction_x << 1;
    scanner_y = y1 + sign_y;
    if (fraction_x == scale_factor_2) {
        scanner_x = x1 + sign_x;
        fraction_x -= scale_factor_1;
    } else {
        scanner_x = x1;
    }

    /* Note (below) the case (qx == f2), where */
    /* the LOS exactly meets the corner of a tile. */
    while (y2 - scanner_y) {
        if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) {
            return false;
        }

        fraction_x += m;

        if (fraction_x < scale_factor_2) {
            scanner_y += sign_y;
        } else if (fraction_x > scale_factor_2) {
            scanner_x += sign_x;
            if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) {
                return false;
            }
            fraction_x -= scale_factor_1;
            scanner_y += sign_y;
        } else {
            scanner_x += sign_x;
            fraction_x -= scale_factor_1;
            scanner_y += sign_y;
        }
    }

    return true;
}

/*
 * breath shape
 */
void breath_shape(PlayerType *player_ptr, const projection_path &path, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, AttributeType typ)
{
    POSITION by = y1;
    POSITION bx = x1;
    int brad = 0;
    int brev = rad * rad / dist;
    int bdis = 0;
    int cdis;
    int path_n = 0;
    int mdis = distance(y1, x1, y2, x2) + rad;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    while (bdis <= mdis) {
        if ((0 < dist) && (path_n < dist)) {
            const auto [ny, nx] = path[path_n];
            POSITION nd = distance(ny, nx, y1, x1);

            if (bdis >= nd) {
                by = ny;
                bx = nx;
                path_n++;
            }
        }

        /* Travel from center outward */
        for (cdis = 0; cdis <= brad; cdis++) {
            for (POSITION y = by - cdis; y <= by + cdis; y++) {
                for (POSITION x = bx - cdis; x <= bx + cdis; x++) {
                    if (!in_bounds(floor_ptr, y, x)) {
                        continue;
                    }
                    if (distance(y1, x1, y, x) != bdis) {
                        continue;
                    }
                    if (distance(by, bx, y, x) != cdis) {
                        continue;
                    }

                    switch (typ) {
                    case AttributeType::LITE:
                    case AttributeType::LITE_WEAK:
                        /* Lights are stopped by opaque terrains */
                        if (!los(player_ptr, by, bx, y, x)) {
                            continue;
                        }
                        break;
                    case AttributeType::DISINTEGRATE:
                        /* Disintegration are stopped only by perma-walls */
                        if (!in_disintegration_range(floor_ptr, by, bx, y, x)) {
                            continue;
                        }
                        break;
                    default:
                        /* Ball explosions are stopped by walls */
                        if (!projectable(player_ptr, by, bx, y, x)) {
                            continue;
                        }
                        break;
                    }

                    gy[*pgrids] = y;
                    gx[*pgrids] = x;
                    (*pgrids)++;
                }
            }
        }

        gm[bdis + 1] = *pgrids;
        brad = rad * (path_n + brev) / (dist + brev);
        bdis++;
    }

    *pgm_rad = bdis;
}
