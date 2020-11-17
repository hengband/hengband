/*!
 * @brief fill_data_type構造体を使ってダンジョンを生成/構成する処理
 * @date 2020/07/24
 * @author Hourier
 */

#include "room/cave-filler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/lake-types.h"
#include "system/floor-type-definition.h"

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

    floor_ptr->grid_array[fill_data.ymin][fill_data.xmin].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmin].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymin][fill_data.xmax].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmax].feat = (s16b)maxsize;
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
        return FALSE;

    floor_ptr->grid_array[y][x].info |= (CAVE_ICKY);
    if (floor_ptr->grid_array[y][x].feat <= c1) {
        if (randint1(100) < 75) {
            floor_ptr->grid_array[y][x].feat = feat1;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info1;
            return TRUE;
        } else {
            floor_ptr->grid_array[y][x].feat = feat2;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info2;
            return TRUE;
        }
    }

    if (floor_ptr->grid_array[y][x].feat <= c2) {
        if (randint1(100) < 75) {
            floor_ptr->grid_array[y][x].feat = feat2;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info2;
            return TRUE;
        } else {
            floor_ptr->grid_array[y][x].feat = feat1;
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
            floor_ptr->grid_array[y][x].info |= info1;
            return TRUE;
        }
    }

    if (floor_ptr->grid_array[y][x].feat <= c3) {
        floor_ptr->grid_array[y][x].feat = feat3;
        floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
        floor_ptr->grid_array[y][x].info |= info3;
        return TRUE;
    }

    place_bold(player_ptr, y, x, GB_OUTER);
    return FALSE;
}

/*
 * Quick and nasty fill routine used to find the connected region
 * of floor in the middle of the grids
 */
static void cave_fill(player_type *player_ptr, POSITION y, POSITION x)
{
    int flow_tail_room = 1;
    int flow_head_room = 0;
    tmp_pos.y[0] = y;
    tmp_pos.x[0] = x;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    while (flow_head_room != flow_tail_room) {
        POSITION ty = tmp_pos.y[flow_head_room];
        POSITION tx = tmp_pos.x[flow_head_room];
        if (++flow_head_room == TEMP_MAX)
            flow_head_room = 0;

        for (int d = 0; d < 8; d++) {
            int old_head = flow_tail_room;
            int j = ty + ddy_ddd[d];
            int i = tx + ddx_ddd[d];
            if (!in_bounds(floor_ptr, j, i)) {
                floor_ptr->grid_array[j][i].info |= CAVE_ICKY;
                continue;
            }

            if ((i <= fill_data.xmin) || (i >= fill_data.xmax) || (j <= fill_data.ymin) || (j >= fill_data.ymax)) {
                floor_ptr->grid_array[j][i].info |= CAVE_ICKY;
                continue;
            }

            if (!hack_isnt_wall(player_ptr, j, i, fill_data.c1, fill_data.c2, fill_data.c3, fill_data.feat1, fill_data.feat2, fill_data.feat3, fill_data.info1,
                    fill_data.info2, fill_data.info3))
                continue;

            tmp_pos.y[flow_tail_room] = (byte)j;
            tmp_pos.x[flow_tail_room] = (byte)i;
            if (++flow_tail_room == TEMP_MAX)
                flow_tail_room = 0;

            if (flow_tail_room == flow_head_room) {
                flow_tail_room = old_head;
                continue;
            }

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

        return FALSE;
    }

    for (int i = 0; i <= xsize; ++i) {
        if ((floor_ptr->grid_array[0 + y0 - yhsize][i + x0 - xhsize].info & CAVE_ICKY) && (room)) {
            place_bold(player_ptr, y0 + 0 - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light)
                floor_ptr->grid_array[y0 + 0 - yhsize][x0 + i - xhsize].info |= (CAVE_GLOW);

            floor_ptr->grid_array[y0 + 0 - yhsize][x0 + i - xhsize].info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + 0 - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + 0 - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        if ((floor_ptr->grid_array[ysize + y0 - yhsize][i + x0 - xhsize].info & CAVE_ICKY) && (room)) {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
            if (light)
                floor_ptr->grid_array[y0 + ysize - yhsize][x0 + i - xhsize].info |= (CAVE_GLOW);

            floor_ptr->grid_array[y0 + ysize - yhsize][x0 + i - xhsize].info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_EXTRA);
        }

        floor_ptr->grid_array[y0 + 0 - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + ysize - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
    }

    for (int i = 1; i < ysize; ++i) {
        if ((floor_ptr->grid_array[i + y0 - yhsize][0 + x0 - xhsize].info & CAVE_ICKY) && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 + 0 - xhsize, GB_OUTER);
            if (light)
                floor_ptr->grid_array[y0 + i - yhsize][x0 + 0 - xhsize].info |= (CAVE_GLOW);

            floor_ptr->grid_array[y0 + i - yhsize][x0 + 0 - xhsize].info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 + 0 - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 + 0 - xhsize, GB_EXTRA);
        }

        if ((floor_ptr->grid_array[i + y0 - yhsize][xsize + x0 - xhsize].info & CAVE_ICKY) && room) {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
            if (light)
                floor_ptr->grid_array[y0 + i - yhsize][x0 + xsize - xhsize].info |= (CAVE_GLOW);

            floor_ptr->grid_array[y0 + i - yhsize][x0 + xsize - xhsize].info |= (CAVE_ROOM);
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_OUTER);
        } else {
            place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_EXTRA);
        }

        floor_ptr->grid_array[y0 + i - yhsize][x0 + 0 - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + i - yhsize][x0 + xsize - xhsize].info &= ~(CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; ++x) {
        for (POSITION y = 1; y < ysize; ++y) {
            if (is_floor_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize) && (floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY)) {
                floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~CAVE_ICKY;
                if (light)
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_GLOW);

                if (room)
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_ROOM);

                continue;
            }

            if (is_outer_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize) && (floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY)) {
                floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY);
                if (light)
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_GLOW);

                if (room) {
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= (CAVE_ROOM);
                } else {
                    place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ROOM);
                }

                continue;
            }

            place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);
            floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
        }
    }

    return TRUE;
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
        return FALSE;
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

        return FALSE;
    }

    for (int i = 0; i <= xsize; ++i) {
        place_bold(player_ptr, y0 + 0 - yhsize, x0 + i - xhsize, GB_EXTRA);
        place_bold(player_ptr, y0 + ysize - yhsize, x0 + i - xhsize, GB_EXTRA);
        floor_ptr->grid_array[y0 + 0 - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + ysize - yhsize][x0 + i - xhsize].info &= ~(CAVE_ICKY);
    }

    for (int i = 1; i < ysize; ++i) {
        place_bold(player_ptr, y0 + i - yhsize, x0 + 0 - xhsize, GB_EXTRA);
        place_bold(player_ptr, y0 + i - yhsize, x0 + xsize - xhsize, GB_EXTRA);
        floor_ptr->grid_array[y0 + i - yhsize][x0 + 0 - xhsize].info &= ~(CAVE_ICKY);
        floor_ptr->grid_array[y0 + i - yhsize][x0 + xsize - xhsize].info &= ~(CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; ++x) {
        for (POSITION y = 1; y < ysize; ++y) {
            if ((!(floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info & CAVE_ICKY)) || is_outer_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize))
                place_bold(player_ptr, y0 + y - yhsize, x0 + x - xhsize, GB_EXTRA);

            floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info &= ~(CAVE_ICKY | CAVE_ROOM);
            if (cave_has_flag_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize, FF_LAVA)) {
                if (!(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= CAVE_GLOW;
            }
        }
    }

    return TRUE;
}
