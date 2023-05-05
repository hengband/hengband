#include "player/player-view.h"
#include "floor/cave.h"
#include "floor/line-of-sight.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g1_c_ptr;
    grid_type *g2_c_ptr;
    g1_c_ptr = &floor_ptr->grid_array[y1][x1];
    g2_c_ptr = &floor_ptr->grid_array[y2][x2];
    bool f1 = (feat_supports_los(g1_c_ptr->feat));
    bool f2 = (feat_supports_los(g2_c_ptr->feat));
    if (!f1 && !f2) {
        return true;
    }

    bool v1 = (f1 && (g1_c_ptr->info & CAVE_VIEW));
    bool v2 = (f2 && (g2_c_ptr->info & CAVE_VIEW));
    if (!v1 && !v2) {
        return true;
    }

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    bool wall = (!feat_supports_los(g_ptr->feat));
    bool z1 = (v1 && (g1_c_ptr->info & CAVE_XTRA));
    bool z2 = (v2 && (g2_c_ptr->info & CAVE_XTRA));
    if (z1 && z2) {
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y, x);
        return wall;
    }

    if (z1) {
        cave_view_hack(floor_ptr, y, x);
        return wall;
    }

    if (v1 && v2) {
        cave_view_hack(floor_ptr, y, x);
        return wall;
    }

    if (wall) {
        cave_view_hack(floor_ptr, y, x);
        return wall;
    }

    if (los(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
        cave_view_hack(floor_ptr, y, x);
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
    // 前回プレイヤーから見えていた座標たちを格納する配列。
    std::vector<Pos2D> points;

    int n, m, d, k, z;
    POSITION y, x;

    int se, sw, ne, nw, es, en, ws, wn;

    int full, over;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    POSITION y_max = floor_ptr->height - 1;
    POSITION x_max = floor_ptr->width - 1;

    grid_type *g_ptr;
    if (view_reduce_view && !floor_ptr->dun_level) {
        full = MAX_PLAYER_SIGHT / 2;
        over = MAX_PLAYER_SIGHT * 3 / 4;
    } else {
        full = MAX_PLAYER_SIGHT;
        over = MAX_PLAYER_SIGHT * 3 / 2;
    }

    for (n = 0; n < floor_ptr->view_n; n++) {
        y = floor_ptr->view_y[n];
        x = floor_ptr->view_x[n];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_VIEW);
        g_ptr->info |= CAVE_TEMP;

        points.emplace_back(y, x);
    }

    floor_ptr->view_n = 0;
    y = player_ptr->y;
    x = player_ptr->x;
    g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->info |= CAVE_XTRA;
    cave_view_hack(floor_ptr, y, x);

    z = full * 2 / 3;
    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y + d, x + d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y + d, x - d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y - d, x + d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    for (d = 1; d <= z; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y - d, x - d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y + d][x];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y + d, x);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    se = sw = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y - d][x];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y - d, x);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    ne = nw = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y][x + d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y, x + d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    es = en = d;
    for (d = 1; d <= full; d++) {
        g_ptr = &floor_ptr->grid_array[y][x - d];
        g_ptr->info |= CAVE_XTRA;
        cave_view_hack(floor_ptr, y, x - d);
        if (!feat_supports_los(g_ptr->feat)) {
            break;
        }
    }

    ws = wn = d;
    for (n = 1; n <= over / 2; n++) {
        POSITION ypn, ymn, xpn, xmn;
        z = over - n - n;
        if (z > full - n) {
            z = full - n;
        }

        while ((z + n + (n >> 1)) > full) {
            z--;
        }

        ypn = y + n;
        ymn = y - n;
        xpn = x + n;
        xmn = x - n;
        if (ypn < y_max) {
            m = std::min(z, y_max - ypn);
            if ((xpn <= x_max) && (n < se)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn + d, xpn, ypn + d - 1, xpn - 1, ypn + d - 1, xpn)) {
                        if (n + d >= se) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                se = k + 1;
            }

            if ((xmn >= 0) && (n < sw)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn + d, xmn, ypn + d - 1, xmn + 1, ypn + d - 1, xmn)) {
                        if (n + d >= sw) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                sw = k + 1;
            }
        }

        if (ymn > 0) {
            m = std::min(z, ymn);
            if ((xpn <= x_max) && (n < ne)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn - d, xpn, ymn - d + 1, xpn - 1, ymn - d + 1, xpn)) {
                        if (n + d >= ne) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                ne = k + 1;
            }

            if ((xmn >= 0) && (n < nw)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn - d, xmn, ymn - d + 1, xmn + 1, ymn - d + 1, xmn)) {
                        if (n + d >= nw) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                nw = k + 1;
            }
        }

        if (xpn < x_max) {
            m = std::min(z, x_max - xpn);
            if ((ypn <= x_max) && (n < es)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn, xpn + d, ypn - 1, xpn + d - 1, ypn, xpn + d - 1)) {
                        if (n + d >= es) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                es = k + 1;
            }

            if ((ymn >= 0) && (n < en)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn, xpn + d, ymn + 1, xpn + d - 1, ymn, xpn + d - 1)) {
                        if (n + d >= en) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                en = k + 1;
            }
        }

        if (xmn > 0) {
            m = std::min(z, xmn);
            if ((ypn <= y_max) && (n < ws)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ypn, xmn - d, ypn - 1, xmn - d + 1, ypn, xmn - d + 1)) {
                        if (n + d >= ws) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                ws = k + 1;
            }

            if ((ymn >= 0) && (n < wn)) {
                for (k = n, d = 1; d <= m; d++) {
                    if (update_view_aux(player_ptr, ymn, xmn - d, ymn + 1, xmn - d + 1, ymn, xmn - d + 1)) {
                        if (n + d >= wn) {
                            break;
                        }
                    } else {
                        k = n + d;
                    }
                }

                wn = k + 1;
            }
        }
    }

    for (n = 0; n < floor_ptr->view_n; n++) {
        y = floor_ptr->view_y[n];
        x = floor_ptr->view_x[n];
        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_XTRA);
        if (g_ptr->info & CAVE_TEMP) {
            continue;
        }

        cave_note_and_redraw_later(floor_ptr, y, x);
    }

    for (const auto &[py, px] : points) {
        g_ptr = &floor_ptr->grid_array[py][px];
        g_ptr->info &= ~(CAVE_TEMP);
        if (g_ptr->info & CAVE_VIEW) {
            continue;
        }

        cave_redraw_later(floor_ptr, py, px);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRedrawingFlag::DELAY_VISIBILITY);
}
