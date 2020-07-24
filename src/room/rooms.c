/*!
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
#include "room/cave-filler.h"
#include "room/lake-types.h"
#include "room/room-generator.h"
#include "room/room-info-table.h"
#include "room/rooms-city.h"
#include "room/rooms-fractal.h"
#include "room/rooms-normal.h"
#include "room/rooms-pit-nest.h"
#include "room/rooms-special.h"
#include "room/rooms-trap.h"
#include "room/rooms-vault.h"
#include "room/treasure-deployment.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"

door_type feat_door[MAX_DOOR_TYPES];

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
        if (block_x + (blocks_wide / 2) <= dun_data->col_rooms / 2) {
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

    if ((by1 < 0) || (by2 > dun_data->row_rooms) || (bx1 < 0) || (bx2 > dun_data->col_rooms))
        return FALSE;

    for (POSITION by = by1; by < by2; by++)
        for (POSITION bx = bx1; bx < bx2; bx++)
            if (dun_data->room_map[by][bx])
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
    if ((dun_data->row_rooms < blocks_high) || (dun_data->col_rooms < blocks_wide))
        return FALSE;

    int candidates = 0;
    for (block_y = dun_data->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dun_data->col_rooms - blocks_wide; block_x >= 0; block_x--) {
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

    for (block_y = dun_data->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dun_data->col_rooms - blocks_wide; block_x >= 0; block_x--) {
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
    if (dun_data->cent_n < CENT_MAX) {
        dun_data->cent[dun_data->cent_n].y = (byte)*y;
        dun_data->cent[dun_data->cent_n].x = (byte)*x;
        dun_data->cent_n++;
    }

    for (POSITION by = by1; by < by2; by++)
        for (POSITION bx = bx1; bx < bx2; bx++)
            dun_data->room_map[by][bx] = TRUE;

    check_room_boundary(player_ptr, *x - width / 2 - 1, *y - height / 2 - 1, *x + (width - 1) / 2 + 1, *y + (height - 1) / 2 + 1);
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
            grid_type *g_ptr;
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
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->info & CAVE_ROOM)
        return;

    g_ptr->info |= CAVE_ROOM;
    feature_type *f_ptr;
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
