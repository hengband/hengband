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

#include "room/rooms-builder.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/cave-filler.h"
#include "room/door-definition.h"
#include "room/lake-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 1マスだけの部屋を作成し、上下左右いずれか一つに隠しドアを配置する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y0 配置したい中心のY座標
 * @param x0 配置したい中心のX座標
 * @details
 * This funtion makes a very small room centred at (x0, y0)
 * This is used in crypts, and random elemental vaults.
 *
 * Note - this should be used only on allocated regions
 * within another room.
 */
void build_small_room(PlayerType *player_ptr, POSITION x0, POSITION y0)
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

/*
 * Builds a cave system in the center of the dungeon.
 */
void build_cavern(PlayerType *player_ptr)
{
    bool light = false;
    bool done = false;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->dun_level <= randint1(50)) && d_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS))
        light = true;

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
        done = generate_fracave(player_ptr, y0 + 1, x0 + 1, xsize, ysize, cutoff, light, false);
    }
}

/*
 * makes a lake/collapsed floor in the center of the dungeon
 */
void build_lake(PlayerType *player_ptr, int type)
{
    if ((type < LAKE_T_LAVA) || (type > LAKE_T_FIRE_VAULT)) {
        msg_format("Invalid lake type (%d)", type);
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    int xsize = floor_ptr->width - 1;
    int ysize = floor_ptr->height - 1;
    int x0 = xsize / 2;
    int y0 = ysize / 2;
    xsize = x0 * 2;
    ysize = y0 * 2;
    bool done = false;
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
void build_room(PlayerType *player_ptr, POSITION x1, POSITION x2, POSITION y1, POSITION y2)
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
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
            auto *g_ptr = &floor_ptr->grid_array[y1 + y][x1 + x];
            if (g_ptr->is_extra()) {
                place_bold(player_ptr, y1 + y, x1 + x, GB_FLOOR);
                g_ptr->info |= (CAVE_ROOM | CAVE_ICKY);
            } else {
                g_ptr->info |= (CAVE_ROOM | CAVE_ICKY);
            }
        }
    }
}

/*
 * Build a town/ castle by using a recursive algorithm.
 * Basically divide each region in a probalistic way to create
 * smaller regions.  When the regions get too small stop.
 *
 * The power variable is a measure of how well defended a region is.
 * This alters the possible choices.
 */
void build_recursive_room(PlayerType *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int power)
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
void add_outer_wall(PlayerType *player_ptr, POSITION x, POSITION y, int light, POSITION x1, POSITION y1, POSITION x2, POSITION y2)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->is_room())
        return;

    g_ptr->info |= CAVE_ROOM;
    feature_type *f_ptr;
    f_ptr = &f_info[g_ptr->feat];
    if (g_ptr->is_floor()) {
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

    if (g_ptr->is_extra()) {
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
        return dx + (dy * h1) / h2;

    if (dy >= 2 * dx)
        return dy + (dx * h1) / h2;

    return ((dx + dy) * 128) / 181 + (dx * dx / (dy * h3) + dy * dy / (dx * h3)) * h4;
}
