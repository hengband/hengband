#include "player/player-view.h"
#include "floor/line-of-sight.h"
#include "game-option/map-screen-options.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/point-2d.h"
#include <vector>

/*
 * Helper function for "update_view()" below
 *
 * We are checking the "viewability" of grid (y,x) by the player.
 *
 * This function assumes that (y,x) is legal (i.e. on the map).
 *
 * Grid (y1,x1) is on the "diagonal" between (player_ptr->y,player_ptr->x) and (y,x)
 * Grid (y2,x2) is "adjacent", also between (player_ptr->y,player_ptr->x) and (y,x).
 *
 * Note that we are using the "CAVE_XTRA" field for marking grids as
 * "easily viewable".  This bit is cleared at the end of "update_view()".
 *
 * This function adds (y,x) to the "viewable set" if necessary.
 *
 * This function now returns "TRUE" if vision is "blocked" by grid (y,x).
 */
static bool update_view_aux(PlayerType *player_ptr, POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    Grid *g1_c_ptr;
    Grid *g2_c_ptr;
    g1_c_ptr = &floor.grid_array[y1][x1];
    g2_c_ptr = &floor.grid_array[y2][x2];
    bool f1 = g1_c_ptr->has_los_terrain();
    bool f2 = g2_c_ptr->has_los_terrain();
    if (!f1 && !f2) {
        return true;
    }

    bool v1 = (f1 && (g1_c_ptr->info & CAVE_VIEW));
    bool v2 = (f2 && (g2_c_ptr->info & CAVE_VIEW));
    if (!v1 && !v2) {
        return true;
    }

    auto &grid = floor.grid_array[y][x];
    bool wall = !grid.has_los_terrain();
    bool z1 = (v1 && (g1_c_ptr->info & CAVE_XTRA));
    bool z2 = (v2 && (g2_c_ptr->info & CAVE_XTRA));
    if (z1 && z2) {
        grid.info |= CAVE_XTRA;
        floor.set_view_at(pos);
        return wall;
    }

    if (z1) {
        floor.set_view_at(pos);
        return wall;
    }

    if (v1 && v2) {
        floor.set_view_at(pos);
        return wall;
    }

    if (wall) {
        floor.set_view_at(pos);
        return wall;
    }

    if (los(floor, player_ptr->get_position(), { y, x })) {
        floor.set_view_at(pos);
        return wall;
    }

    return true;
}

/*
 * Calculate the viewable space
 *
 *  1: Process the player
 *  1a: The player is always (easily) viewable
 *  2: Process the diagonals
 *  2a: The diagonals are (easily) viewable up to the first wall
 *  2b: But never go more than 2/3 of the "full" distance
 *  3: Process the main axes
 *  3a: The main axes are (easily) viewable up to the first wall
 *  3b: But never go more than the "full" distance
 *  4: Process sequential "strips" in each of the eight octants
 *  4a: Each strip runs along the previous strip
 *  4b: The main axes are "previous" to the first strip
 *  4c: Process both "sides" of each "direction" of each strip
 *  4c1: Each side aborts as soon as possible
 *  4c2: Each side tells the next strip how far it has to check
 */
void update_view(PlayerType *player_ptr)
{
    int m, d, k;
    int se, sw, ne, nw, es, en, ws, wn;

    int full, over;

    auto &floor = *player_ptr->current_floor_ptr;
    POSITION y_max = floor.height - 1;
    POSITION x_max = floor.width - 1;

    if (view_reduce_view && !floor.is_underground()) {
        full = MAX_PLAYER_SIGHT / 2;
        over = MAX_PLAYER_SIGHT * 3 / 4;
    } else {
        full = MAX_PLAYER_SIGHT;
        over = MAX_PLAYER_SIGHT * 3 / 2;
    }

    const auto points = floor.reset_view();
    auto y = player_ptr->y;
    auto x = player_ptr->x;
    auto &grid_player = floor.grid_array[y][x];
    grid_player.info |= CAVE_XTRA;

    floor.set_view_at({ y, x });
    auto z = full * 2 / 3;
    for (d = 1; d <= z; d++) {
        auto &grid = floor.grid_array[y + d][x + d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(d, d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        auto &grid = floor.grid_array[y + d][x - d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(d, -d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        auto &grid = floor.grid_array[y - d][x + d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(-d, d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        auto &grid = floor.grid_array[y - d][x - d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(-d, -d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    for (d = 1; d <= full; d++) {
        auto &grid = floor.grid_array[y + d][x];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(d, 0));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    se = sw = d;
    for (d = 1; d <= full; d++) {
        auto &grid = floor.grid_array[y - d][x];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(-d, 0));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    ne = nw = d;
    for (d = 1; d <= full; d++) {
        auto &grid = floor.grid_array[y][x + d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(0, d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    es = en = d;
    for (d = 1; d <= full; d++) {
        auto &grid = floor.grid_array[y][x - d];
        grid.info |= CAVE_XTRA;
        floor.set_view_at(Pos2D(y, x) + Pos2DVec(0, -d));
        if (!grid.has_los_terrain()) {
            break;
        }
    }

    ws = wn = d;
    for (auto i = 1; i <= over / 2; i++) {
        POSITION ypn, ymn, xpn, xmn;
        z = over - i - i;
        if (z > full - i) {
            z = full - i;
        }

        while ((z + i + (i >> 1)) > full) {
            z--;
        }

        ypn = y + i;
        ymn = y - i;
        xpn = x + i;
        xmn = x - i;
        if (ypn < y_max) {
            m = std::min(z, y_max - ypn);
            if ((xpn <= x_max) && (i < se)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn + d, xpn, ypn + d - 1, xpn - 1, ypn + d - 1, xpn)) {
                        if (i + d >= se) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                se = k + 1;
            }

            if ((xmn >= 0) && (i < sw)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn + d, xmn, ypn + d - 1, xmn + 1, ypn + d - 1, xmn)) {
                        if (i + d >= sw) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                sw = k + 1;
            }
        }

        if (ymn > 0) {
            m = std::min(z, ymn);
            if ((xpn <= x_max) && (i < ne)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn - d, xpn, ymn - d + 1, xpn - 1, ymn - d + 1, xpn)) {
                        if (i + d >= ne) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                ne = k + 1;
            }

            if ((xmn >= 0) && (i < nw)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn - d, xmn, ymn - d + 1, xmn + 1, ymn - d + 1, xmn)) {
                        if (i + d >= nw) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                nw = k + 1;
            }
        }

        if (xpn < x_max) {
            m = std::min(z, x_max - xpn);
            if ((ypn <= x_max) && (i < es)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn, xpn + d, ypn - 1, xpn + d - 1, ypn, xpn + d - 1)) {
                        if (i + d >= es) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                es = k + 1;
            }

            if ((ymn >= 0) && (i < en)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn, xpn + d, ymn + 1, xpn + d - 1, ymn, xpn + d - 1)) {
                        if (i + d >= en) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                en = k + 1;
            }
        }

        if (xmn > 0) {
            m = std::min(z, xmn);
            if ((ypn <= y_max) && (i < ws)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn, xmn - d, ypn - 1, xmn - d + 1, ypn, xmn - d + 1)) {
                        if (i + d >= ws) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                ws = k + 1;
            }

            if ((ymn >= 0) && (i < wn)) {
                for (k = i, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn, xmn - d, ymn + 1, xmn - d + 1, ymn, xmn - d + 1)) {
                        if (i + d >= wn) {
                            break;
                        }
                    } else {
                        k = i + d;
                    }
                }

                wn = k + 1;
            }
        }
    }

    for (auto i = 0; i < floor.view_n; i++) {
        y = floor.view_y[i];
        x = floor.view_x[i];
        const Pos2D pos(y, x);
        auto &grid = floor.get_grid(pos);
        grid.info &= ~(CAVE_XTRA);
        if (grid.info & CAVE_TEMP) {
            continue;
        }

        floor.set_note_and_redraw_at(pos);
    }

    for (const auto &pos : points) {
        auto &grid = floor.get_grid(pos);
        grid.info &= ~(CAVE_TEMP);
        if (grid.info & CAVE_VIEW) {
            continue;
        }

        floor.set_redraw_at(pos);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::DELAY_VISIBILITY);
}
