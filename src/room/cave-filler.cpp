/*!
 * @brief fill_data_type構造体を使ってダンジョンを生成/構成する処理
 * @date 2020/07/24
 * @author Hourier
 */

#include "room/cave-filler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor//geometry.h"
#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/lake-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/point-2d.h"
#include <queue>

typedef struct fill_data_type {
    POSITION xmin;
    POSITION ymin;
    POSITION xmax;
    POSITION ymax;

    /* cutoffs */
    int c1;
    int c2;
    int c3;

    /* features to fill with */
    FEAT_IDX feat1;
    FEAT_IDX feat2;
    FEAT_IDX feat3;

    int info1;
    int info2;
    int info3;

    /* number of filled squares */
    int amount;
} fill_data_type;

static fill_data_type fill_data;

/*!
 * Store routine for the fractal floor generator
 * this routine probably should be an inline function or a macro.
 */
static void store_height(floor_type *floor_ptr, POSITION x, POSITION y, FEAT_IDX val)
{
    if (((x == fill_data.xmin) || (y == fill_data.ymin) || (x == fill_data.xmax) || (y == fill_data.ymax)) && (val <= fill_data.c1))
        val = fill_data.c1 + 1;

    floor_ptr->grid_array[y][x].feat = val;
    return;
}

void generate_hmap(floor_type *floor_ptr, POSITION y0, POSITION x0, POSITION xsiz, POSITION ysiz, int grd, int roug, int cutoff)
{
    POSITION xsize = xsiz;
    POSITION ysize = ysiz;

    if (xsize > 254)
        xsize = 254;

    if (xsize < 4)
        xsize = 4;

    if (ysize > 254)
        ysize = 254;

    if (ysize < 4)
        ysize = 4;

    POSITION xhsize = xsize / 2;
    POSITION yhsize = ysize / 2;
    xsize = xhsize * 2;
    ysize = yhsize * 2;

    fill_data.xmin = x0 - xhsize;
    fill_data.ymin = y0 - yhsize;
    fill_data.xmax = x0 + xhsize;
    fill_data.ymax = y0 + yhsize;
    fill_data.c1 = cutoff;
    POSITION diagsize = 362;
    POSITION maxsize = (xsize > ysize) ? xsize : ysize;
    for (POSITION i = 0; i <= xsize; i++) {
        for (POSITION j = 0; j <= ysize; j++) {
            floor_ptr->grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].feat = -1;
            floor_ptr->grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].info &= ~(CAVE_ICKY);
        }
    }

    floor_ptr->grid_array[fill_data.ymin][fill_data.xmin].feat = (int16_t)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmin].feat = (int16_t)maxsize;
    floor_ptr->grid_array[fill_data.ymin][fill_data.xmax].feat = (int16_t)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmax].feat = (int16_t)maxsize;
    floor_ptr->grid_array[y0][x0].feat = 0;
    POSITION xstep = xsize * 256;
    POSITION xhstep = xsize * 256;
    POSITION ystep = ysize * 256;
    POSITION yhstep = ysize * 256;
    POSITION xxsize = xsize * 256;
    POSITION yysize = ysize * 256;
    while ((xhstep > 256) || (yhstep > 256)) {
        xstep = xhstep;
        xhstep /= 2;
        ystep = yhstep;
        yhstep /= 2;
        POSITION xstep2 = xstep / 256;
        POSITION ystep2 = ystep / 256;
        POSITION xhstep2 = xhstep / 256;
        POSITION yhstep2 = yhstep / 256;
        for (POSITION i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (POSITION j = 0; j <= yysize; j += ystep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor_ptr->grid_array[jj][ii].feat != -1)
                    continue;

                if (xhstep2 > grd) {
                    store_height(floor_ptr, ii, jj, randint1(maxsize));
                    continue;
                }

                store_height(floor_ptr, ii, jj,
                    (floor_ptr->grid_array[jj][fill_data.xmin + (i - xhstep) / 256].feat + floor_ptr->grid_array[jj][fill_data.xmin + (i + xhstep) / 256].feat)
                            / 2
                        + (randint1(xstep2) - xhstep2) * roug / 16);
            }
        }

        for (POSITION j = yhstep; j <= yysize - yhstep; j += ystep) {
            for (POSITION i = 0; i <= xxsize; i += xstep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor_ptr->grid_array[jj][ii].feat != -1)
                    continue;

                if (xhstep2 > grd) {
                    store_height(floor_ptr, ii, jj, randint1(maxsize));
                    continue;
                }

                store_height(floor_ptr, ii, jj,
                    (floor_ptr->grid_array[fill_data.ymin + (j - yhstep) / 256][ii].feat + floor_ptr->grid_array[fill_data.ymin + (j + yhstep) / 256][ii].feat)
                            / 2
                        + (randint1(ystep2) - yhstep2) * roug / 16);
            }
        }

        for (POSITION i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (POSITION j = yhstep; j <= yysize - yhstep; j += ystep) {
                POSITION ii = i / 256 + fill_data.xmin;
                POSITION jj = j / 256 + fill_data.ymin;
                if (floor_ptr->grid_array[jj][ii].feat != -1)
                    continue;

                if (xhstep2 > grd) {
                    store_height(floor_ptr, ii, jj, randint1(maxsize));
                    continue;
                }

                POSITION xm = fill_data.xmin + (i - xhstep) / 256;
                POSITION xp = fill_data.xmin + (i + xhstep) / 256;
                POSITION ym = fill_data.ymin + (j - yhstep) / 256;
                POSITION yp = fill_data.ymin + (j + yhstep) / 256;
                store_height(floor_ptr, ii, jj,
                    (floor_ptr->grid_array[ym][xm].feat + floor_ptr->grid_array[yp][xm].feat + floor_ptr->grid_array[ym][xp].feat
                        + floor_ptr->grid_array[yp][xp].feat)
                            / 4
                        + (randint1(xstep2) - xhstep2) * (diagsize / 16) / 256 * roug);
            }
        }
    }
}

static bool hack_isnt_wall(player_type *player_ptr, POSITION y, POSITION x, int c1, int c2, int c3, FEAT_IDX feat1, FEAT_IDX feat2, FEAT_IDX feat3,
    BIT_FLAGS info1, BIT_FLAGS info2, BIT_FLAGS info3)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->grid_array[y][x].info & CAVE_ICKY)
        return false;

    floor_ptr->grid_array[y][x].info |= (CAVE_ICKY);
    if (floor_ptr->grid_array[y][x].feat <= c1) {
        if (randint1(100) < 75) {
            floor_ptr->grid_array[y][x].feat = feat1;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info1;
            return true;
        } else {
            floor_ptr->grid_array[y][x].feat = feat2;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info2;
            return true;
        }
    }

    if (floor_ptr->grid_array[y][x].feat <= c2) {
        if (randint1(100) < 75) {
            floor_ptr->grid_array[y][x].feat = feat2;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info2;
            return true;
        } else {
            floor_ptr->grid_array[y][x].feat = feat1;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info1;
            return true;
        }
    }

    if (floor_ptr->grid_array[y][x].feat <= c3) {
        floor_ptr->grid_array[y][x].feat = feat3;
        floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
        floor_ptr->grid_array[y][x].info |= info3;
        return true;
    }

    place_bold(player_ptr, y, x, GB_OUTER);
    return false;
}

/*
 * Quick and nasty fill routine used to find the connected region
 * of floor in the middle of the grids
 */
static void cave_fill(player_type *player_ptr, const POSITION y, const POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;

    // 幅優先探索用のキュー。
    std::queue<Pos2D> que;
    que.emplace(y, x);

    while (!que.empty()) {
        const auto [y_cur, x_cur] = que.front();
        que.pop();

        for (int d = 0; d < 8; d++) {
            int y_to = y_cur + ddy_ddd[d];
            int x_to = x_cur + ddx_ddd[d];
            if (!in_bounds(floor_ptr, y_to, x_to)) {
                floor_ptr->grid_array[y_to][x_to].info |= CAVE_ICKY;
                continue;
            }

            if ((x_to <= fill_data.xmin) || (x_to >= fill_data.xmax) || (y_to <= fill_data.ymin) || (y_to >= fill_data.ymax)) {
                floor_ptr->grid_array[y_to][x_to].info |= CAVE_ICKY;
                continue;
            }

            if (!hack_isnt_wall(player_ptr, y_to, x_to, fill_data.c1, fill_data.c2, fill_data.c3, fill_data.feat1, fill_data.feat2, fill_data.feat3,
                    fill_data.info1, fill_data.info2, fill_data.info3))
                continue;

            que.emplace(y_to, x_to);

            (fill_data.amount)++;
        }
    }
}

bool generate_fracave(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int cutoff, bool light, bool room)
{
    POSITION xhsize = xsize / 2;
    POSITION yhsize = ysize / 2;
    fill_data.c1 = cutoff;
    fill_data.c2 = 0;
    fill_data.c3 = 0;
    fill_data.feat1 = feat_ground_type[randint0(100)];
    fill_data.feat2 = feat_ground_type[randint0(100)];
    fill_data.feat3 = feat_ground_type[randint0(100)];
    fill_data.info1 = CAVE_FLOOR;
    fill_data.info2 = CAVE_FLOOR;
    fill_data.info3 = CAVE_FLOOR;
    fill_data.amount = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    cave_fill(player_ptr, (byte)y0, (byte)x0);
    if (fill_data.amount < 10) {
        for (POSITION x = 0; x <= xsize; ++x) {
            for (POSITION y = 0; y <= ysize; ++y) {
                place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
                floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
            }
        }

        return false;
    }

    for (int i = 0; i <= xsize; ++i) {
        auto *g_ptr1 = &floor_ptr->grid_array[y0 - yhsize][i + x0 - xhsize];
        if (g_ptr1->is_icky() && (room)) {
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light)
                floor_ptr->grid_array[y0 - yhsize][x0 + i - xhsize].info |= (CAVE_GLOW);

            g_ptr1->info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        auto *g_ptr2 = &floor_ptr->grid_array[ysize + y0 - yhsize][i + x0 - xhsize];
        if (g_ptr2->is_icky() && (room)) {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light)
                g_ptr2->info |= (CAVE_GLOW);

            g_ptr2->info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        g_ptr1->info &= ~(CAVE_ICKY);
        g_ptr2->info &= ~(CAVE_ICKY);
    }

    for (int i = 1; i < ysize; ++i) {
        auto *g_ptr1 = &floor_ptr->grid_array[i + y0 - yhsize][x0 - xhsize];
        if (g_ptr1->is_icky() && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_OUTER);
            if (light)
                g_ptr1->info |= (CAVE_GLOW);

            g_ptr1->info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_EXTRA);
        }

        auto *g_ptr2 = &floor_ptr->grid_array[i + y0 - yhsize][xsize + x0 - xhsize];
        if (g_ptr2->is_icky() && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
            if (light)
                g_ptr2->info |= (CAVE_GLOW);

            g_ptr2->info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_EXTRA);
        }

        g_ptr1->info &= ~(CAVE_ICKY);
        g_ptr2->info &= ~(CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; ++x) {
        for (POSITION y = 1; y < ysize; ++y) {
            auto *g_ptr1 = &floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize];
            if (g_ptr1->is_floor() && g_ptr1->is_icky()) {
                g_ptr1->info &= ~CAVE_ICKY;
                if (light)
                    g_ptr1->info |= (CAVE_GLOW);

                if (room)
                    g_ptr1->info |= (CAVE_ROOM);

                continue;
            }

            auto *g_ptr2 = &floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize];
            if (g_ptr2->is_outer() && g_ptr2->is_icky()) {
                g_ptr2->info &= ~(CAVE_ICKY);
                if (light)
                    g_ptr2->info |= (CAVE_GLOW);

                if (room) {
                    g_ptr2->info |= (CAVE_ROOM);
                } else {
                    place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
                    g_ptr2->info &= ~(CAVE_ROOM);
                }

                continue;
            }

            place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
            g_ptr2->info &= ~(CAVE_ICKY | CAVE_ROOM);
        }
    }

    return true;
}

bool generate_lake(player_type *player_ptr, POSITION y0, POSITION x0, POSITION xsize, POSITION ysize, int c1, int c2, int c3, int type)
{
    FEAT_IDX feat1, feat2, feat3;
    POSITION xhsize = xsize / 2;
    POSITION yhsize = ysize / 2;
    switch (type) {
    case LAKE_T_LAVA: /* Lava */
        feat1 = feat_deep_lava;
        feat2 = feat_shallow_lava;
        feat3 = feat_ground_type[randint0(100)];
        break;
    case LAKE_T_WATER: /* Water */
        feat1 = feat_deep_water;
        feat2 = feat_shallow_water;
        feat3 = feat_ground_type[randint0(100)];
        break;
    case LAKE_T_CAVE: /* Collapsed floor_ptr->grid_array */
        feat1 = feat_ground_type[randint0(100)];
        feat2 = feat_ground_type[randint0(100)];
        feat3 = feat_rubble;
        break;
    case LAKE_T_EARTH_VAULT: /* Earth vault */
        feat1 = feat_rubble;
        feat2 = feat_ground_type[randint0(100)];
        feat3 = feat_rubble;
        break;
    case LAKE_T_AIR_VAULT: /* Air vault */
        feat1 = feat_grass;
        feat2 = feat_tree;
        feat3 = feat_grass;
        break;
    case LAKE_T_WATER_VAULT: /* Water vault */
        feat1 = feat_shallow_water;
        feat2 = feat_deep_water;
        feat3 = feat_shallow_water;
        break;
    case LAKE_T_FIRE_VAULT: /* Fire Vault */
        feat1 = feat_shallow_lava;
        feat2 = feat_deep_lava;
        feat3 = feat_shallow_lava;
        break;
    default:
        return false;
    }

    fill_data.c1 = c1;
    fill_data.c2 = c2;
    fill_data.c3 = c3;
    fill_data.feat1 = feat1;
    fill_data.feat2 = feat2;
    fill_data.feat3 = feat3;
    fill_data.info1 = 0;
    fill_data.info2 = 0;
    fill_data.info3 = 0;
    fill_data.amount = 0;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    cave_fill(player_ptr, (byte)y0, (byte)x0);
    if (fill_data.amount < 10) {
        for (POSITION x = 0; x <= xsize; ++x) {
            for (POSITION y = 0; y <= ysize; ++y) {
                place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_FLOOR);
                floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY);
            }
        }

        return false;
    }

    for (int i = 0; i <= xsize; ++i) {
        place_bold(player_ptr, y0 + 0 - yhsize, x0 + i - xhsize, GB_EXTRA);
        place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_EXTRA);
        floor_ptr->grid_array[y0 - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + ysize - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
    }

    for (int i = 1; i < ysize; ++i) {
        place_bold(player_ptr, y0 + i - yhsize, x0 - xhsize, GB_EXTRA);
        place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_EXTRA);
        floor_ptr->grid_array[y0 + i - yhsize][x0 - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + i - yhsize][x0 + xsize - xhsize].info &= ~(CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; ++x) {
        for (POSITION y = 1; y < ysize; ++y) {
            auto *g_ptr = &floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize];
            if (!g_ptr->is_icky() || g_ptr->is_outer())
                place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);

            floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
            if (cave_has_flag_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize, FF_LAVA)) {
                if (d_info[floor_ptr->dungeon_idx].flags.has_not(DF::DARKNESS))
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= CAVE_GLOW;
            }
        }
    }

    return true;
}
