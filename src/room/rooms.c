/*!
 * @file rooms.c
 * @brief ダンジョンフロアの部屋生成処理 / make rooms. Used by generate.c when creating dungeons.
 * @date 2014/01/06
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 * @details
 * Room building routines.\n
 *\n
 * Room types:\n
 *   1 -- normal\n
 *   2 -- overlapping\n
 *   3 -- cross shaped\n
 *   4 -- large room with features\n
 *   5 -- monster nests\n
 *   6 -- monster pits\n
 *   7 -- simple vaults\n
 *   8 -- greater vaults\n
 *   9 -- fractal caves\n
 *  10 -- random vaults\n
 *  11 -- circular rooms\n
 *  12 -- crypts\n
 *  13 -- trapped monster pits\n
 *  14 -- trapped room\n
 *  15 -- glass room\n
 *  16 -- underground arcade\n
 *\n
 * Some functions are used to determine if the given monster\n
 * is appropriate for inclusion in a monster nest or monster pit or\n
 * the given type.\n
 *\n
 * None of the pits/nests are allowed to include "unique" monsters.\n
 */

#include "room/rooms.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/floor-generate.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-apply-magic.h"
#include "room/rooms-city.h"
#include "room/rooms-fractal.h"
#include "room/rooms-normal.h"
#include "room/rooms-pit-nest.h"
#include "room/rooms-special.h"
#include "room/rooms-trap.h"
#include "room/rooms-vault.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

/*!
 * 各部屋タイプの生成比定義
 *[from SAngband (originally from OAngband)]\n
 *\n
 * Table of values that control how many times each type of room will\n
 * appear.  Each type of room has its own row, and each column\n
 * corresponds to dungeon levels 0, 10, 20, and so on.  The final\n
 * value is the minimum depth the room can appear at.  -LM-\n
 *\n
 * Level 101 and below use the values for level 100.\n
 *\n
 * Rooms with lots of monsters or loot may not be generated if the\n
 * object or monster lists are already nearly full.  Rooms will not\n
 * appear above their minimum depth.  Tiny levels will not have space\n
 * for all the rooms you ask for.\n
 */

static room_info_type room_info_normal[ROOM_T_MAX] = {
    /* Depth */
    /*  0  10  20  30  40  50  60  70  80  90 100  min limit */
    { { 999, 900, 800, 700, 600, 500, 400, 300, 200, 100, 0 }, 0 }, /*NORMAL   */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 1 }, /*OVERLAP  */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 3 }, /*CROSS    */
    { { 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 }, 3 }, /*INNER_F  */
    { { 0, 1, 1, 1, 2, 3, 5, 6, 8, 10, 13 }, 10 }, /*NEST     */
    { { 0, 1, 1, 2, 3, 4, 6, 8, 10, 13, 16 }, 10 }, /*PIT      */
    { { 0, 1, 1, 1, 2, 2, 3, 5, 6, 8, 10 }, 10 }, /*LESSER_V */
    { { 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4 }, 20 }, /*GREATER_V*/
    { { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 999 }, 10 }, /*FRACAVE  */
    { { 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 }, 10 }, /*RANDOM_V */
    { { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40 }, 3 }, /*OVAL     */
    { { 1, 6, 12, 18, 24, 30, 36, 42, 48, 54, 60 }, 10 }, /*CRYPT    */
    { { 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 8 }, 20 }, /*TRAP_PIT */
    { { 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 8 }, 20 }, /*TRAP     */
    { { 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2 }, 40 }, /*GLASS    */
    { { 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3 }, 1 }, /*ARCADE   */
    { { 1, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80 }, 1 }, /*FIX      */
};

/*! 部屋の生成処理順 / Build rooms in descending order of difficulty. */
static byte room_build_order[ROOM_T_MAX] = {
    ROOM_T_GREATER_VAULT,
    ROOM_T_ARCADE,
    ROOM_T_RANDOM_VAULT,
    ROOM_T_LESSER_VAULT,
    ROOM_T_TRAP_PIT,
    ROOM_T_PIT,
    ROOM_T_NEST,
    ROOM_T_TRAP,
    ROOM_T_GLASS,
    ROOM_T_INNER_FEAT,
    ROOM_T_FIXED,
    ROOM_T_OVAL,
    ROOM_T_CRYPT,
    ROOM_T_OVERLAP,
    ROOM_T_CROSS,
    ROOM_T_FRACAVE,
    ROOM_T_NORMAL,
};

/*!
 * @brief 1マスだけの部屋を作成し、上下左右いずれか一つに隠しドアを配置する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y0 配置したい中心のY座標
 * @param x0 配置したい中心のX座標
 * @details
 * This funtion makes a very small room centred at (x0, y0)
 * This is used in crypts, and random elemental vaults.
 *
 * Note - this should be used only on allocated regions
 * within another room.
 */
void build_small_room(player_type *player_ptr, POSITION x0, POSITION y0)
{
    for (POSITION y = y0 - 1; y <= y0 + 1; y++) {
        place_bold(player_ptr, y, x0 - 1, GB_INNER);
        place_bold(player_ptr, y, x0 + 1, GB_INNER);
    }

    for (POSITION x = x0 - 1; x <= x0 + 1; x++) {
        place_bold(player_ptr, y0 - 1, x, GB_INNER);
        place_bold(player_ptr, y0 + 1, x, GB_INNER);
    }

    switch (randint0(4)) {
    case 0:
        place_secret_door(player_ptr, y0, x0 - 1, DOOR_DEFAULT);
        break;
    case 1:
        place_secret_door(player_ptr, y0, x0 + 1, DOOR_DEFAULT);
        break;
    case 2:
        place_secret_door(player_ptr, y0 - 1, x0, DOOR_DEFAULT);
        break;
    case 3:
        place_secret_door(player_ptr, y0 + 1, x0, DOOR_DEFAULT);
        break;
    }

    player_ptr->current_floor_ptr->grid_array[y0][x0].mimic = 0;
    place_bold(player_ptr, y0, x0, GB_FLOOR);
}

/*!
 * @brief
 * 指定範囲に通路が通っていることを確認した上で床で埋める
 * This function tunnels around a room if it will cut off part of a grid system.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x1 範囲の左端
 * @param y1 範囲の上端
 * @param x2 範囲の右端
 * @param y2 範囲の下端
 * @return なし
 */
static void check_room_boundary(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2)
{
    bool old_is_floor;
    bool new_is_floor;
    int count = 0;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    old_is_floor = get_is_floor(floor_ptr, x1 - 1, y1);

    for (POSITION x = x1; x <= x2; x++) {
        new_is_floor = get_is_floor(floor_ptr, x, y1 - 1);
        if (new_is_floor != old_is_floor)
            count++;

        old_is_floor = new_is_floor;
    }

    for (POSITION y = y1; y <= y2; y++) {
        new_is_floor = get_is_floor(floor_ptr, x2 + 1, y);
        if (new_is_floor != old_is_floor)
            count++;

        old_is_floor = new_is_floor;
    }

    for (POSITION x = x2; x >= x1; x--) {
        new_is_floor = get_is_floor(floor_ptr, x, y2 + 1);
        if (new_is_floor != old_is_floor)
            count++;

        old_is_floor = new_is_floor;
    }

    for (POSITION y = y2; y >= y1; y--) {
        new_is_floor = get_is_floor(floor_ptr, x1 - 1, y);
        if (new_is_floor != old_is_floor)
            count++;

        old_is_floor = new_is_floor;
    }

    if (count <= 2)
        return;

    for (POSITION y = y1; y <= y2; y++)
        for (POSITION x = x1; x <= x2; x++)
            set_floor(player_ptr, x, y);
}

/*!
 * @brief
 * find_space()の予備処理として部屋の生成が可能かを判定する /
 * Helper function for find_space(). Is this a good location?
 * @param blocks_high 範囲の高さ
 * @param blocks_wide 範囲の幅
 * @param block_y 範囲の上端
 * @param block_x 範囲の左端
 * @return なし
 */
static bool find_space_aux(POSITION blocks_high, POSITION blocks_wide, POSITION block_y, POSITION block_x)
{
    if (blocks_wide < 3) {
        if ((blocks_wide == 2) && (block_x % 3) == 2)
            return FALSE;
    } else if ((blocks_wide % 3) == 0) {
        if ((block_x % 3) != 0)
            return FALSE;
    } else {
        if (block_x + (blocks_wide / 2) <= dun->col_rooms / 2) {
            if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
                return FALSE;
            if ((block_x % 3) == 1)
                return FALSE;
        } else {
            if (((block_x % 3) == 2) && ((blocks_wide % 3) == 2))
                return FALSE;
            if ((block_x % 3) == 1)
                return FALSE;
        }
    }

    POSITION by1 = block_y;
    POSITION bx1 = block_x;
    POSITION by2 = block_y + blocks_high;
    POSITION bx2 = block_x + blocks_wide;

    if ((by1 < 0) || (by2 > dun->row_rooms) || (bx1 < 0) || (bx2 > dun->col_rooms))
        return FALSE;

    for (POSITION by = by1; by < by2; by++)
        for (POSITION bx = bx1; bx < bx2; bx++)
            if (dun->room_map[by][bx])
                return FALSE;

    return TRUE;
}

/*!
 * @brief 部屋生成が可能なスペースを確保する / Find a good spot for the next room.  -LM-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 部屋の生成が可能な中心Y座標を返す参照ポインタ
 * @param x 部屋の生成が可能な中心X座標を返す参照ポインタ
 * @param height 確保したい領域の高さ
 * @param width 確保したい領域の幅
 * @return 所定の範囲が確保できた場合TRUEを返す
 * @details
 * Find and allocate a free space in the dungeon large enough to hold\n
 * the room calling this function.\n
 *\n
 * We allocate space in 11x11 blocks, but want to make sure that rooms\n
 * align neatly on the standard screen.  Therefore, we make them use\n
 * blocks in few 11x33 rectangles as possible.\n
 *\n
 * Be careful to include the edges of the room in height and width!\n
 *\n
 * Return TRUE and values for the center of the room if all went well.\n
 * Otherwise, return FALSE.\n
 */
bool find_space(player_type *player_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width)
{
    int pick;
    POSITION block_y = 0;
    POSITION block_x = 0;
    POSITION blocks_high = 1 + ((height - 1) / BLOCK_HGT);
    POSITION blocks_wide = 1 + ((width - 1) / BLOCK_WID);
    if ((dun->row_rooms < blocks_high) || (dun->col_rooms < blocks_wide))
        return FALSE;

    int candidates = 0;
    for (block_y = dun->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dun->col_rooms - blocks_wide; block_x >= 0; block_x--) {
            if (find_space_aux(blocks_high, blocks_wide, block_y, block_x)) {
                /* Find a valid place */
                candidates++;
            }
        }
    }

    if (!candidates)
        return FALSE;

    if (!(d_info[player_ptr->current_floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE))
        pick = randint1(candidates);
    else
        pick = candidates / 2 + 1;

    for (block_y = dun->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dun->col_rooms - blocks_wide; block_x >= 0; block_x--) {
            if (find_space_aux(blocks_high, blocks_wide, block_y, block_x)) {
                pick--;
                if (!pick)
                    break;
            }
        }

        if (!pick)
            break;
    }

    POSITION by1 = block_y;
    POSITION bx1 = block_x;
    POSITION by2 = block_y + blocks_high;
    POSITION bx2 = block_x + blocks_wide;
    *y = ((by1 + by2) * BLOCK_HGT) / 2;
    *x = ((bx1 + bx2) * BLOCK_WID) / 2;
    if (dun->cent_n < CENT_MAX) {
        dun->cent[dun->cent_n].y = (byte)*y;
        dun->cent[dun->cent_n].x = (byte)*x;
        dun->cent_n++;
    }

    for (POSITION by = by1; by < by2; by++)
        for (POSITION bx = bx1; bx < bx2; bx++)
            dun->room_map[by][bx] = TRUE;

    check_room_boundary(player_ptr, *x - width / 2 - 1, *y - height / 2 - 1, *x + (width - 1) / 2 + 1, *y + (height - 1) / 2 + 1);
    return TRUE;
}

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
    POSITION xhstep, yhstep;
    POSITION xstep2, xhstep2, ystep2, yhstep2;
    POSITION i, j, ii, jj;
    POSITION xm, xp, ym, yp;
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
    for (i = 0; i <= xsize; i++) {
        for (j = 0; j <= ysize; j++) {
            floor_ptr->grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].feat = -1;
            floor_ptr->grid_array[(int)(fill_data.ymin + j)][(int)(fill_data.xmin + i)].info &= ~(CAVE_ICKY);
        }
    }

    floor_ptr->grid_array[fill_data.ymin][fill_data.xmin].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmin].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymin][fill_data.xmax].feat = (s16b)maxsize;
    floor_ptr->grid_array[fill_data.ymax][fill_data.xmax].feat = (s16b)maxsize;
    floor_ptr->grid_array[y0][x0].feat = 0;
    POSITION xstep = xhstep = xsize * 256;
    POSITION ystep = yhstep = ysize * 256;
    POSITION xxsize = xsize * 256;
    POSITION yysize = ysize * 256;
    while ((xhstep > 256) || (yhstep > 256)) {
        xstep = xhstep;
        xhstep /= 2;
        ystep = yhstep;
        yhstep /= 2;
        xstep2 = xstep / 256;
        ystep2 = ystep / 256;
        xhstep2 = xhstep / 256;
        yhstep2 = yhstep / 256;
        for (i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (j = 0; j <= yysize; j += ystep) {
                ii = i / 256 + fill_data.xmin;
                jj = j / 256 + fill_data.ymin;
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

        for (j = yhstep; j <= yysize - yhstep; j += ystep) {
            for (i = 0; i <= xxsize; i += xstep) {
                ii = i / 256 + fill_data.xmin;
                jj = j / 256 + fill_data.ymin;
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

        for (i = xhstep; i <= xxsize - xhstep; i += xstep) {
            for (j = yhstep; j <= yysize - yhstep; j += ystep) {
                ii = i / 256 + fill_data.xmin;
                jj = j / 256 + fill_data.ymin;
                if (floor_ptr->grid_array[jj][ii].feat != -1)
                    continue;

                if (xhstep2 > grd) {
                    store_height(floor_ptr, ii, jj, randint1(maxsize));
                    continue;
                }

                xm = fill_data.xmin + (i - xhstep) / 256;
                xp = fill_data.xmin + (i + xhstep) / 256;
                ym = fill_data.ymin + (j - yhstep) / 256;
                yp = fill_data.ymin + (j + yhstep) / 256;
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

/*
 * Builds a cave system in the center of the dungeon.
 */
void build_cavern(player_type *player_ptr)
{
    bool light = FALSE;
    bool done = FALSE;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->dun_level <= randint1(50)) && !(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
        light = TRUE;

    POSITION xsize = floor_ptr->width - 1;
    POSITION ysize = floor_ptr->height - 1;
    POSITION x0 = xsize / 2;
    POSITION y0 = ysize / 2;

    /* Paranoia: make size even */
    xsize = x0 * 2;
    ysize = y0 * 2;

    while (!done) {
        int grd = randint1(4) + 4;
        int roug = randint1(8) * randint1(4);
        int cutoff = xsize / 2;
        generate_hmap(floor_ptr, y0 + 1, x0 + 1, xsize, ysize, grd, roug, cutoff);
        done = generate_fracave(player_ptr, y0 + 1, x0 + 1, xsize, ysize, cutoff, light, FALSE);
    }
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
            if (cave_have_flag_bold(floor_ptr, y0 + y - yhsize, x0 + x - xhsize, FF_LAVA)) {
                if (!(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
                    floor_ptr->grid_array[y0 + y - yhsize][x0 + x - xhsize].info |= CAVE_GLOW;
            }
        }
    }

    return TRUE;
}

/*
 * makes a lake/collapsed floor in the center of the dungeon
 */
void build_lake(player_type *player_ptr, int type)
{
    if ((type < LAKE_T_LAVA) || (type > LAKE_T_FIRE_VAULT)) {
        msg_format("Invalid lake type (%d)", type);
        return;
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int xsize = floor_ptr->width - 1;
    int ysize = floor_ptr->height - 1;
    int x0 = xsize / 2;
    int y0 = ysize / 2;
    xsize = x0 * 2;
    ysize = y0 * 2;
    bool done = FALSE;
    while (!done) {
        int grd = randint1(3) + 4;
        int roug = randint1(8) * randint1(4);
        int c3 = 3 * xsize / 4;
        int c1 = randint0(c3 / 2) + randint0(c3 / 2) - 5;
        int c2 = (c1 + c3) / 2;
        generate_hmap(floor_ptr, y0 + 1, x0 + 1, xsize, ysize, grd, roug, c3);
        done = generate_lake(player_ptr, y0 + 1, x0 + 1, xsize, ysize, c1, c2, c3, type);
    }
}

/*
 * Routine that fills the empty areas of a room with treasure and monsters.
 */
void fill_treasure(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2, int difficulty)
{
    POSITION cx = (x1 + x2) / 2;
    POSITION cy = (y1 + y2) / 2;
    POSITION size = abs(x2 - x1) + abs(y2 - y1);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION x = x1; x <= x2; x++) {
        for (POSITION y = y1; y <= y2; y++) {
            s32b value = ((((s32b)(distance(cx, cy, x, y))) * 100) / size) + randint1(10) - difficulty;
            if ((randint1(100) - difficulty * 3) > 50)
                value = 20;

            if (!is_floor_bold(floor_ptr, y, x) && (!cave_have_flag_bold(floor_ptr, y, x, FF_PLACE) || !cave_have_flag_bold(floor_ptr, y, x, FF_DROP)))
                continue;

            if (value < 0) {
                floor_ptr->monster_level = floor_ptr->base_level + 40;
                place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor_ptr->monster_level = floor_ptr->base_level;
                floor_ptr->object_level = floor_ptr->base_level + 20;
                place_object(player_ptr, y, x, AM_GOOD);
                floor_ptr->object_level = floor_ptr->base_level;
            } else if (value < 5) {
                floor_ptr->monster_level = floor_ptr->base_level + 20;
                place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor_ptr->monster_level = floor_ptr->base_level;
                floor_ptr->object_level = floor_ptr->base_level + 10;
                place_object(player_ptr, y, x, AM_GOOD);
                floor_ptr->object_level = floor_ptr->base_level;
            } else if (value < 10) {
                floor_ptr->monster_level = floor_ptr->base_level + 9;
                place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor_ptr->monster_level = floor_ptr->base_level;
            } else if (value < 17) {
            } else if (value < 23) {
                if (randint0(100) < 25) {
                    place_object(player_ptr, y, x, 0L);
                } else {
                    place_trap(player_ptr, y, x);
                }
            } else if (value < 30) {
                floor_ptr->monster_level = floor_ptr->base_level + 5;
                place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                floor_ptr->monster_level = floor_ptr->base_level;
                place_trap(player_ptr, y, x);
            } else if (value < 40) {
                if (randint0(100) < 50) {
                    floor_ptr->monster_level = floor_ptr->base_level + 3;
                    place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                    floor_ptr->monster_level = floor_ptr->base_level;
                }

                if (randint0(100) < 50) {
                    floor_ptr->object_level = floor_ptr->base_level + 7;
                    place_object(player_ptr, y, x, 0L);
                    floor_ptr->object_level = floor_ptr->base_level;
                }
            } else if (value < 50) {
                place_trap(player_ptr, y, x);
            } else {
                if (randint0(100) < 20) {
                    place_monster(player_ptr, y, x, (PM_ALLOW_SLEEP | PM_ALLOW_GROUP));
                } else if (randint0(100) < 50) {
                    place_trap(player_ptr, y, x);
                } else if (randint0(100) < 50) {
                    place_object(player_ptr, y, x, 0L);
                }
            }
        }
    }
}

/*
 * Overlay a rectangular room given its bounds
 * This routine is used by build_room_vault
 * The area inside the walls is not touched:
 * only granite is removed- normal walls stay
 */
void build_room(player_type *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2)
{
    int temp;
    if ((x1 == x2) || (y1 == y2))
        return;

    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }

    POSITION xsize = x2 - x1;
    POSITION ysize = y2 - y1;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i <= xsize; i++) {
        place_bold(player_ptr, y1, x1 + i, GB_OUTER_NOPERM);
        floor_ptr->grid_array[y1][x1 + i].info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, y2, x1 + i, GB_OUTER_NOPERM);
        floor_ptr->grid_array[y2][x1 + i].info |= (CAVE_ROOM | CAVE_ICKY);
    }

    for (int i = 1; i < ysize; i++) {
        place_bold(player_ptr, y1 + i, x1, GB_OUTER_NOPERM);
        floor_ptr->grid_array[y1 + i][x1].info |= (CAVE_ROOM | CAVE_ICKY);
        place_bold(player_ptr, y1 + i, x2, GB_OUTER_NOPERM);
        floor_ptr->grid_array[y1 + i][x2].info |= (CAVE_ROOM | CAVE_ICKY);
    }

    for (POSITION x = 1; x < xsize; x++) {
        for (POSITION y = 1; y < ysize; y++) {
            if (is_extra_bold(floor_ptr, y1 + y, x1 + x)) {
                place_bold(player_ptr, y1 + y, x1 + x, GB_FLOOR);
                floor_ptr->grid_array[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
            } else {
                floor_ptr->grid_array[y1 + y][x1 + x].info |= (CAVE_ROOM | CAVE_ICKY);
            }
        }
    }
}

/*
 * maze vault -- rectangular labyrinthine rooms
 *
 * maze vault uses two routines:
 *    r_visit - a recursive routine that builds the labyrinth
 *    build_maze_vault - a driver routine that calls r_visit and adds
 *                   monsters, traps and treasure
 *
 * The labyrinth is built by creating a spanning tree of a graph.
 * The graph vertices are at
 *    (x, y) = (2j + x1, 2k + y1)   j = 0,...,m-1    k = 0,...,n-1
 * and the edges are the vertical and horizontal nearest neighbors.
 *
 * The spanning tree is created by performing a suitably randomized
 * depth-first traversal of the graph. The only adjustable parameter
 * is the randint0(3) below; it governs the relative density of
 * twists and turns in the labyrinth: smaller number, more twists.
 */
void r_visit(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int node, DIRECTION dir, int *visited)
{
    int adj[4];
    int m = (x2 - x1) / 2 + 1;
    int n = (y2 - y1) / 2 + 1;
    visited[node] = 1;
    int x = 2 * (node % m) + x1;
    int y = 2 * (node / m) + y1;
    place_bold(player_ptr, y, x, GB_FLOOR);

    if (one_in_(3)) {
        for (int i = 0; i < 4; i++)
            adj[i] = i;

        for (int i = 0; i < 4; i++) {
            int j = randint0(4);
            int temp = adj[i];
            adj[i] = adj[j];
            adj[j] = temp;
        }

        dir = adj[0];
    } else {
        adj[0] = dir;
        for (int i = 1; i < 4; i++)
            adj[i] = i;

        for (int i = 1; i < 4; i++) {
            int j = 1 + randint0(3);
            int temp = adj[i];
            adj[i] = adj[j];
            adj[j] = temp;
        }
    }

    for (int i = 0; i < 4; i++) {
        switch (adj[i]) {
        case 0:
            /* (0,+) - check for bottom boundary */
            if ((node / m < n - 1) && (visited[node + m] == 0)) {
                place_bold(player_ptr, y + 1, x, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node + m, dir, visited);
            }
            break;
        case 1:
            /* (0,-) - check for top boundary */
            if ((node / m > 0) && (visited[node - m] == 0)) {
                place_bold(player_ptr, y - 1, x, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node - m, dir, visited);
            }
            break;
        case 2:
            /* (+,0) - check for right boundary */
            if ((node % m < m - 1) && (visited[node + 1] == 0)) {
                place_bold(player_ptr, y, x + 1, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node + 1, dir, visited);
            }
            break;
        case 3:
            /* (-,0) - check for left boundary */
            if ((node % m > 0) && (visited[node - 1] == 0)) {
                place_bold(player_ptr, y, x - 1, GB_FLOOR);
                r_visit(player_ptr, y1, x1, y2, x2, node - 1, dir, visited);
            }
        }
    }
}

void build_maze_vault(player_type *player_ptr, POSITION x0, POSITION y0, POSITION xsize, POSITION ysize, bool is_vault)
{
    grid_type *g_ptr;
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("迷路ランダムVaultを生成しました。", "Maze Vault."));
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    bool light = ((floor_ptr->dun_level <= randint1(25)) && is_vault && !(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS));
    POSITION dy = ysize / 2 - 1;
    POSITION dx = xsize / 2 - 1;
    POSITION y1 = y0 - dy;
    POSITION x1 = x0 - dx;
    POSITION y2 = y0 + dy;
    POSITION x2 = x0 + dx;
    for (POSITION y = y1 - 1; y <= y2 + 1; y++) {
        for (POSITION x = x1 - 1; x <= x2 + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info |= CAVE_ROOM;
            if (is_vault)
                g_ptr->info |= CAVE_ICKY;
            if ((x == x1 - 1) || (x == x2 + 1) || (y == y1 - 1) || (y == y2 + 1)) {
                place_grid(player_ptr, g_ptr, GB_OUTER);
            } else if (!is_vault) {
                place_grid(player_ptr, g_ptr, GB_EXTRA);
            } else {
                place_grid(player_ptr, g_ptr, GB_INNER);
            }

            if (light)
                g_ptr->info |= (CAVE_GLOW);
        }
    }

    int m = dx + 1;
    int n = dy + 1;
    int num_vertices = m * n;

    int *visited;
    C_MAKE(visited, num_vertices, int);
    r_visit(player_ptr, y1, x1, y2, x2, randint0(num_vertices), 0, visited);
    if (is_vault)
        fill_treasure(player_ptr, x1, x2, y1, y2, randint1(5));

    C_KILL(visited, num_vertices, int);
}

/*
 * Build a town/ castle by using a recursive algorithm.
 * Basically divide each region in a probalistic way to create
 * smaller regions.  When the regions get too small stop.
 *
 * The power variable is a measure of how well defended a region is.
 * This alters the possible choices.
 */
void build_recursive_room(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power)
{
    POSITION xsize = x2 - x1;
    POSITION ysize = y2 - y1;

    int choice;
    if ((power < 3) && (xsize > 12) && (ysize > 12)) {
        choice = 1;
    } else {
        if (power < 10) {
            if ((randint1(10) > 2) && (xsize < 8) && (ysize < 8)) {
                choice = 4;
            } else {
                choice = randint1(2) + 1;
            }
        } else {
            choice = randint1(3) + 1;
        }
    }

    switch (choice) {
    case 1: {
        /* Outer walls */
        int x;
        int y;
        for (x = x1; x <= x2; x++) {
            place_bold(player_ptr, y1, x, GB_OUTER);
            place_bold(player_ptr, y2, x, GB_OUTER);
        }

        for (y = y1 + 1; y < y2; y++) {
            place_bold(player_ptr, y, x1, GB_OUTER);
            place_bold(player_ptr, y, x2, GB_OUTER);
        }

        if (one_in_(2)) {
            y = randint1(ysize) + y1;
            place_bold(player_ptr, y, x1, GB_FLOOR);
            place_bold(player_ptr, y, x2, GB_FLOOR);
        } else {
            x = randint1(xsize) + x1;
            place_bold(player_ptr, y1, x, GB_FLOOR);
            place_bold(player_ptr, y2, x, GB_FLOOR);
        }

        int t1 = randint1(ysize / 3) + y1;
        int t2 = y2 - randint1(ysize / 3);
        int t3 = randint1(xsize / 3) + x1;
        int t4 = x2 - randint1(xsize / 3);

        /* Do outside areas */
        build_recursive_room(player_ptr, x1 + 1, y1 + 1, x2 - 1, t1, power + 1);
        build_recursive_room(player_ptr, x1 + 1, t2, x2 - 1, y2, power + 1);
        build_recursive_room(player_ptr, x1 + 1, t1 + 1, t3, t2 - 1, power + 3);
        build_recursive_room(player_ptr, t4, t1 + 1, x2 - 1, t2 - 1, power + 3);

        x1 = t3;
        x2 = t4;
        y1 = t1;
        y2 = t2;
        xsize = x2 - x1;
        ysize = y2 - y1;
        power += 2;
    }
        /* Fall through */
    case 4: {
        /* Try to build a room */
        if ((xsize < 3) || (ysize < 3)) {
            for (int y = y1; y < y2; y++) {
                for (int x = x1; x < x2; x++) {
                    place_bold(player_ptr, y, x, GB_INNER);
                }
            }

            return;
        }

        for (int x = x1 + 1; x <= x2 - 1; x++) {
            place_bold(player_ptr, y1 + 1, x, GB_INNER);
            place_bold(player_ptr, y2 - 1, x, GB_INNER);
        }

        for (int y = y1 + 1; y <= y2 - 1; y++) {
            place_bold(player_ptr, y, x1 + 1, GB_INNER);
            place_bold(player_ptr, y, x2 - 1, GB_INNER);
        }

        int y = randint1(ysize - 3) + y1 + 1;
        if (one_in_(2)) {
            /* left */
            place_bold(player_ptr, y, x1 + 1, GB_FLOOR);
        } else {
            /* right */
            place_bold(player_ptr, y, x2 - 1, GB_FLOOR);
        }

        build_recursive_room(player_ptr, x1 + 2, y1 + 2, x2 - 2, y2 - 2, power + 3);
        break;
    }
    case 2: {
        /* Try and divide vertically */
        if (xsize < 3) {
            for (int y = y1; y < y2; y++) {
                for (int x = x1; x < x2; x++) {
                    place_bold(player_ptr, y, x, GB_INNER);
                }
            }
            return;
        }

        int t1 = randint1(xsize - 2) + x1 + 1;
        build_recursive_room(player_ptr, x1, y1, t1, y2, power - 2);
        build_recursive_room(player_ptr, t1 + 1, y1, x2, y2, power - 2);
        break;
    }
    case 3: {
        /* Try and divide horizontally */
        if (ysize < 3) {
            for (int y = y1; y < y2; y++)
                for (int x = x1; x < x2; x++)
                    place_bold(player_ptr, y, x, GB_INNER);

            return;
        }

        int t1 = randint1(ysize - 2) + y1 + 1;
        build_recursive_room(player_ptr, x1, y1, x2, t1, power - 2);
        build_recursive_room(player_ptr, x1, t1 + 1, x2, y2, power - 2);
        break;
    }
    }
}

/*
 * Add outer wall to a floored region
 * Note: no range checking is done so must be inside dungeon
 * This routine also stomps on doors
 */
void add_outer_wall(player_type *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2)
{
    grid_type *g_ptr;
    feature_type *f_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return;

    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->info & CAVE_ROOM)
        return;

    g_ptr->info |= CAVE_ROOM;
    f_ptr = &f_info[g_ptr->feat];
    if (is_floor_bold(floor_ptr, y, x)) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if ((x + i >= x1) && (x + i <= x2) && (y + j >= y1) && (y + j <= y2)) {
                    add_outer_wall(player_ptr, x + i, y + j, light, x1, y1, x2, y2);
                    if (light)
                        g_ptr->info |= CAVE_GLOW;
                }
            }
        }

        return;
    }

    if (is_extra_bold(floor_ptr, y, x)) {
        place_bold(player_ptr, y, x, GB_OUTER);
        if (light)
            g_ptr->info |= CAVE_GLOW;

        return;
    }

    if (permanent_wall(f_ptr)) {
        if (light)
            g_ptr->info |= CAVE_GLOW;
    }
}

/*
 * Hacked distance formula - gives the 'wrong' answer.
 * Used to build crypts
 */
POSITION dist2(POSITION x1, POSITION y1, POSITION x2, POSITION y2, POSITION h1, POSITION h2, POSITION h3, POSITION h4)
{
    POSITION dx = abs(x2 - x1);
    POSITION dy = abs(y2 - y1);
    if (dx >= 2 * dy)
        return (dx + (dy * h1) / h2);

    if (dy >= 2 * dx)
        return (dy + (dx * h1) / h2);

    return (((dx + dy) * 128) / 181 + (dx * dx / (dy * h3) + dy * dy / (dx * h3)) * h4);
}

/* Create a new floor room with optional light */
void generate_room_floor(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int light)
{
    grid_type *g_ptr;
    for (POSITION y = y1; y <= y2; y++) {
        for (POSITION x = x1; x <= x2; x++) {
            g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light)
                g_ptr->info |= (CAVE_GLOW);
        }
    }
}

void generate_fill_perm_bold(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    for (POSITION y = y1; y <= y2; y++)
        for (POSITION x = x1; x <= x2; x++)
            place_bold(player_ptr, y, x, GB_INNER_PERM);
}

/*!
 * @brief 与えられた部屋型IDに応じて部屋の生成処理分岐を行い結果を返す / Attempt to build a room of the given type at the given block
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param type 部屋型ID
 * @note that we restrict the number of "crowded" rooms to reduce the chance of overflowing the monster list during level creation.
 * @return 部屋の生成に成功した場合 TRUE を返す。
 */
static bool room_build(player_type *player_ptr, EFFECT_ID typ)
{
    switch (typ) {
    case ROOM_T_NORMAL:
        return build_type1(player_ptr);
    case ROOM_T_OVERLAP:
        return build_type2(player_ptr);
    case ROOM_T_CROSS:
        return build_type3(player_ptr);
    case ROOM_T_INNER_FEAT:
        return build_type4(player_ptr);
    case ROOM_T_NEST:
        return build_type5(player_ptr);
    case ROOM_T_PIT:
        return build_type6(player_ptr);
    case ROOM_T_LESSER_VAULT:
        return build_type7(player_ptr);
    case ROOM_T_GREATER_VAULT:
        return build_type8(player_ptr);
    case ROOM_T_FRACAVE:
        return build_type9(player_ptr);
    case ROOM_T_RANDOM_VAULT:
        return build_type10(player_ptr);
    case ROOM_T_OVAL:
        return build_type11(player_ptr);
    case ROOM_T_CRYPT:
        return build_type12(player_ptr);
    case ROOM_T_TRAP_PIT:
        return build_type13(player_ptr);
    case ROOM_T_TRAP:
        return build_type14(player_ptr);
    case ROOM_T_GLASS:
        return build_type15(player_ptr);
    case ROOM_T_ARCADE:
        return build_type16(player_ptr);
    case ROOM_T_FIXED:
        return build_type17(player_ptr);
    default:
        return FALSE;
    }
}

/*!
 * @brief 指定した部屋の生成確率を別の部屋に加算し、指定した部屋の生成率を0にする
 * @param dst 確率を移す先の部屋種ID
 * @param src 確率を与える元の部屋種ID
 */
#define MOVE_PLIST(dst, src) (prob_list[dst] += prob_list[src], prob_list[src] = 0)

/*!
 * @brief 部屋生成処理のメインルーチン(Sangbandを経由してOangbandからの実装を引用) / Generate rooms in dungeon.  Build bigger rooms at first.　[from SAngband
 * (originally from OAngband)]
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 部屋生成に成功した場合 TRUE を返す。
 */
bool generate_rooms(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    bool remain;
    int crowded = 0;
    int prob_list[ROOM_T_MAX];
    int rooms_built = 0;
    int area_size = 100 * (floor_ptr->height * floor_ptr->width) / (MAX_HGT * MAX_WID);
    int level_index = MIN(10, div_round(floor_ptr->dun_level, 10));
    s16b room_num[ROOM_T_MAX];
    int dun_rooms = DUN_ROOMS_MAX * area_size / 100;
    room_info_type *room_info_ptr = room_info_normal;
    for (int i = 0; i < ROOM_T_MAX; i++) {
        if (floor_ptr->dun_level < room_info_ptr[i].min_level)
            prob_list[i] = 0;
        else
            prob_list[i] = room_info_ptr[i].prob[level_index];
    }

    /*!
     * @details ダンジョンにBEGINNER、CHAMELEON、SMALLESTいずれのフラグもなく、
     * かつ「常に通常でない部屋を生成する」フラグがONならば、
     * GRATER_VAULTのみを生成対象とする。 / Ironman sees only Greater Vaults
     */
    if (ironman_rooms && !((d_info[floor_ptr->dungeon_idx].flags1 & (DF1_BEGINNER | DF1_CHAMELEON | DF1_SMALLEST)))) {
        for (int i = 0; i < ROOM_T_MAX; i++) {
            if (i == ROOM_T_GREATER_VAULT)
                prob_list[i] = 1;
            else
                prob_list[i] = 0;
        }
    } else if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_VAULT) {
        /*! @details ダンジョンにNO_VAULTフラグがあるならば、LESSER_VAULT / GREATER_VAULT/ RANDOM_VAULTを除外 / Forbidden vaults */
        prob_list[ROOM_T_LESSER_VAULT] = 0;
        prob_list[ROOM_T_GREATER_VAULT] = 0;
        prob_list[ROOM_T_RANDOM_VAULT] = 0;
    }

    /*! @details ダンジョンにBEGINNERフラグがあるならば、FIXED_ROOMを除外 / Forbidden vaults */
    if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_BEGINNER)
        prob_list[ROOM_T_FIXED] = 0;

    /*! @details ダンジョンにNO_CAVEフラグがある場合、FRACAVEの生成枠がNORMALに与えられる。CRIPT、OVALの生成枠がINNER_Fに与えられる。/ NO_CAVE dungeon */
    if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) {
        MOVE_PLIST(ROOM_T_NORMAL, ROOM_T_FRACAVE);
        MOVE_PLIST(ROOM_T_INNER_FEAT, ROOM_T_CRYPT);
        MOVE_PLIST(ROOM_T_INNER_FEAT, ROOM_T_OVAL);
    } else if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_CAVE) {
        /*! @details ダンジョンにCAVEフラグがある場合、NORMALの生成枠がFRACAVEに与えられる。/ CAVE dungeon (Orc floor_ptr->grid_array etc.) */
        MOVE_PLIST(ROOM_T_FRACAVE, ROOM_T_NORMAL);
    } else if (dun->cavern || dun->empty_level) {
        /*! @details ダンジョンの基本地形が最初から渓谷かアリーナ型の場合 FRACAVE は生成から除外。 /  No caves when a (random) cavern exists: they look bad */
        prob_list[ROOM_T_FRACAVE] = 0;
    }

    /*! @details ダンジョンに最初からGLASS_ROOMフラグがある場合、GLASS を生成から除外。/ Forbidden glass rooms */
    if (!(d_info[floor_ptr->dungeon_idx].flags1 & DF1_GLASS_ROOM))
        prob_list[ROOM_T_GLASS] = 0;

    /*! @details ARCADEは同フラグがダンジョンにないと生成されない。 / Forbidden glass rooms */
    if (!(d_info[floor_ptr->dungeon_idx].flags1 & DF1_ARCADE))
        prob_list[ROOM_T_ARCADE] = 0;

    int total_prob = 0;
    for (int i = 0; i < ROOM_T_MAX; i++) {
        room_num[i] = 0;
        total_prob += prob_list[i];
    }

    for (int i = dun_rooms; i > 0; i--) {
        int room_type;
        int rand = randint0(total_prob);
        for (room_type = 0; room_type < ROOM_T_MAX; room_type++) {
            if (rand < prob_list[room_type])
                break;
            else
                rand -= prob_list[room_type];
        }

        if (room_type >= ROOM_T_MAX)
            room_type = ROOM_T_NORMAL;

        room_num[room_type]++;
        switch (room_type) {
        case ROOM_T_NEST:
        case ROOM_T_PIT:
        case ROOM_T_LESSER_VAULT:
        case ROOM_T_TRAP_PIT:
        case ROOM_T_GLASS:
        case ROOM_T_ARCADE:
            /* Large room */
            i -= 2;
            break;
        case ROOM_T_GREATER_VAULT:
        case ROOM_T_RANDOM_VAULT:
            /* Largest room */
            i -= 3;
            break;
        }
    }

    while (TRUE) {
        remain = FALSE;
        for (int i = 0; i < ROOM_T_MAX; i++) {
            int room_type = room_build_order[i];
            if (!room_num[room_type])
                continue;

            room_num[room_type]--;
            if (!room_build(player_ptr, room_type))
                continue;

            rooms_built++;
            remain = TRUE;
            switch (room_type) {
            case ROOM_T_PIT:
            case ROOM_T_NEST:
            case ROOM_T_TRAP_PIT:
                if (++crowded >= 2) {
                    room_num[ROOM_T_PIT] = 0;
                    room_num[ROOM_T_NEST] = 0;
                    room_num[ROOM_T_TRAP_PIT] = 0;
                }

                break;
            case ROOM_T_ARCADE:
                room_num[ROOM_T_ARCADE] = 0;
                break;
            }
        }

        if (!remain)
            break;
    }

    /*! @details 部屋生成数が2未満の場合生成失敗を返す */
    if (rooms_built < 2) {
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("部屋数が2未満でした。生成を再試行します。", "Number of rooms was under 2. Retry."), rooms_built);
        return FALSE;
    }

    msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("このダンジョンの部屋数は %d です。", "Number of Rooms: %d"), rooms_built);
    return TRUE;
}
