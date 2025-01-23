/*!
 * @brief 魔法による距離やエリアの計算
 * @date 2014/07/10
 * @author Ben Harrison, James E. Wilson, Robert A. Koeneke, deskull and Hourier
 */

#include "spell/range-calc.h"
#include "effect/attribute-types.h"
#include "floor/cave.h"
#include "floor/line-of-sight.h"
#include "grid/grid.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
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
    POSITION pd = Grid::calc_distance({ y1, x1 }, { y, x });
    POSITION nd = Grid::calc_distance({ y1, x1 }, { y2, x2 });

    if (pd > nd) {
        return Grid::calc_distance({ y, x }, { y2, x2 });
    }

    nd = ((nd) ? ((py * ny + px * nx) / nd) : 0);
    return nd >= 0 ? nd : 0 - nd;
}

/*
 *
 * Modified version of los() for calculation of disintegration balls.
 * Disintegration effects are stopped by permanent walls.
 */
bool in_disintegration_range(const FloorType *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
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
void breath_shape(PlayerType *player_ptr, const ProjectionPath &path, int dist, int *pgrids, std::span<Pos2D> positions, std::span<int> gm, int *pgm_rad, int rad, const Pos2D &pos_source, const Pos2D &pos_target, AttributeType typ)
{
    const auto brev = rad * rad / dist;
    auto by = pos_source.y;
    auto bx = pos_source.x;
    auto brad = 0;
    auto bdis = 0;
    auto path_n = 0;
    const auto mdis = Grid::calc_distance(pos_source, pos_target) + rad;
    int cdis;
    const auto &floor = *player_ptr->current_floor_ptr;
    while (bdis <= mdis) {
        if ((0 < dist) && (path_n < dist)) {
            const auto &pos_path = path[path_n];
            POSITION nd = Grid::calc_distance(pos_path, pos_source);

            if (bdis >= nd) {
                by = pos_path.y;
                bx = pos_path.x;
                path_n++;
            }
        }

        /* Travel from center outward */
        const Pos2D pos_breath(by, bx);
        for (cdis = 0; cdis <= brad; cdis++) {
            for (auto y = pos_breath.y - cdis; y <= pos_breath.y + cdis; y++) {
                for (auto x = pos_breath.x - cdis; x <= pos_breath.x + cdis; x++) {
                    const Pos2D pos(y, x);
                    if (!in_bounds(&floor, pos.y, pos.x)) {
                        continue;
                    }
                    if (Grid::calc_distance(pos_source, pos) != bdis) {
                        continue;
                    }
                    if (Grid::calc_distance(pos_breath, pos) != cdis) {
                        continue;
                    }

                    switch (typ) {
                    case AttributeType::LITE:
                    case AttributeType::LITE_WEAK:
                        /* Lights are stopped by opaque terrains */
                        if (!los(floor, pos_breath, pos)) {
                            continue;
                        }
                        break;
                    case AttributeType::DISINTEGRATE:
                        /* Disintegration are stopped only by perma-walls */
                        if (!in_disintegration_range(&floor, pos_breath.y, pos_breath.x, pos.y, pos.x)) {
                            continue;
                        }
                        break;
                    default:
                        /* Ball explosions are stopped by walls */
                        if (!projectable(player_ptr, pos_breath, pos)) {
                            continue;
                        }
                        break;
                    }

                    positions[*pgrids] = pos;
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
