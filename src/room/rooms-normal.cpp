#include "room/rooms-normal.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/geometry.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/stair.h"
#include "grid/trap.h"
#include "room/door-definition.h"
#include "room/rooms-builder.h"
#include "room/space-finder.h"
#include "room/vault-builder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief タイプ1の部屋…通常可変長方形の部屋を生成する / Type 1 -- normal rectangular rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type1(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, y2, x2, yval, xval;
    POSITION y1, x1, xsize, ysize;

    bool light;

    grid_type *g_ptr;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool curtain = (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN)) && one_in_(dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 48 : 512);

    /* Pick a room size */
    y1 = randint1(4);
    x1 = randint1(11);
    y2 = randint1(3);
    x2 = randint1(11);

    xsize = x1 + x2 + 1;
    ysize = y1 + y2 + 1;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2)) {
        /* Limit to the minimum room size, and retry */
        y1 = 1;
        x1 = 1;
        y2 = 1;
        x2 = 1;

        xsize = x1 + x2 + 1;
        ysize = y1 + y2 + 1;

        /* Find and reserve some space in the dungeon.  Get center of room. */
        if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2)) {
            return false;
        }
    }

    /* Choose lite or dark */
    light = ((floor_ptr->dun_level <= randint1(25)) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    y1 = yval - ysize / 2;
    x1 = xval - xsize / 2;
    y2 = yval + (ysize - 1) / 2;
    x2 = xval + (xsize - 1) / 2;

    /* Place a full floor under the room */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Hack -- Occasional curtained room */
    if (curtain && (y2 - y1 > 2) && (x2 - x1 > 2)) {
        for (y = y1; y <= y2; y++) {
            g_ptr = &floor_ptr->grid_array[y][x1];
            g_ptr->feat = feat_door[DOOR_CURTAIN].closed;
            g_ptr->info &= ~(CAVE_MASK);
            g_ptr = &floor_ptr->grid_array[y][x2];
            g_ptr->feat = feat_door[DOOR_CURTAIN].closed;
            g_ptr->info &= ~(CAVE_MASK);
        }
        for (x = x1; x <= x2; x++) {
            g_ptr = &floor_ptr->grid_array[y1][x];
            g_ptr->feat = feat_door[DOOR_CURTAIN].closed;
            g_ptr->info &= ~(CAVE_MASK);
            g_ptr = &floor_ptr->grid_array[y2][x];
            g_ptr->feat = feat_door[DOOR_CURTAIN].closed;
            g_ptr->info &= ~(CAVE_MASK);
        }
    }

    /* Hack -- Occasional pillar room */
    if (one_in_(20)) {
        for (y = y1; y <= y2; y += 2) {
            for (x = x1; x <= x2; x += 2) {
                g_ptr = &floor_ptr->grid_array[y][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }
        }
    }

    /* Hack -- Occasional room with four pillars */
    else if (one_in_(20)) {
        if ((y1 + 4 < y2) && (x1 + 4 < x2)) {
            g_ptr = &floor_ptr->grid_array[y1 + 1][x1 + 1];
            place_grid(player_ptr, g_ptr, GB_INNER);

            g_ptr = &floor_ptr->grid_array[y1 + 1][x2 - 1];
            place_grid(player_ptr, g_ptr, GB_INNER);

            g_ptr = &floor_ptr->grid_array[y2 - 1][x1 + 1];
            place_grid(player_ptr, g_ptr, GB_INNER);

            g_ptr = &floor_ptr->grid_array[y2 - 1][x2 - 1];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }
    }

    /* Hack -- Occasional ragged-edge room */
    else if (one_in_(50)) {
        for (y = y1 + 2; y <= y2 - 2; y += 2) {
            g_ptr = &floor_ptr->grid_array[y][x1];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y][x2];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }
        for (x = x1 + 2; x <= x2 - 2; x += 2) {
            g_ptr = &floor_ptr->grid_array[y1][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y2][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }
    }
    /* Hack -- Occasional divided room */
    else if (one_in_(50)) {
        bool curtain2 = (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN)) && one_in_(dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 2 : 128);

        if (randint1(100) < 50) {
            /* Horizontal wall */
            for (x = x1; x <= x2; x++) {
                place_bold(player_ptr, yval, x, GB_INNER);
                if (curtain2) {
                    floor_ptr->grid_array[yval][x].feat = feat_door[DOOR_CURTAIN].closed;
                }
            }

            /* Prevent edge of wall from being tunneled */
            place_bold(player_ptr, yval, x1 - 1, GB_SOLID);
            place_bold(player_ptr, yval, x2 + 1, GB_SOLID);
        } else {
            /* Vertical wall */
            for (y = y1; y <= y2; y++) {
                place_bold(player_ptr, y, xval, GB_INNER);
                if (curtain2) {
                    floor_ptr->grid_array[y][xval].feat = feat_door[DOOR_CURTAIN].closed;
                }
            }

            /* Prevent edge of wall from being tunneled */
            place_bold(player_ptr, y1 - 1, xval, GB_SOLID);
            place_bold(player_ptr, y2 + 1, xval, GB_SOLID);
        }

        place_random_door(player_ptr, yval, xval, true);
        if (curtain2) {
            floor_ptr->grid_array[yval][xval].feat = feat_door[DOOR_CURTAIN].closed;
        }
    }

    return true;
}

/*!
 * @brief タイプ2の部屋…二重長方形の部屋を生成する / Type 2 -- Overlapping rectangular rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type2(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, xval, yval;
    POSITION y1a, x1a, y2a, x2a;
    POSITION y1b, x1b, y2b, x2b;
    bool light;
    grid_type *g_ptr;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    /* Choose lite or dark */
    light = ((floor_ptr->dun_level <= randint1(25)) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));

    /* Determine extents of the first room */
    y1a = yval - randint1(4);
    y2a = yval + randint1(3);
    x1a = xval - randint1(11);
    x2a = xval + randint1(10);

    /* Determine extents of the second room */
    y1b = yval - randint1(3);
    y2b = yval + randint1(4);
    x1b = xval - randint1(10);
    x2b = xval + randint1(11);

    /* Place a full floor for room "a" */
    for (y = y1a - 1; y <= y2a + 1; y++) {
        for (x = x1a - 1; x <= x2a + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Place a full floor for room "b" */
    for (y = y1b - 1; y <= y2b + 1; y++) {
        for (x = x1b - 1; x <= x2b + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (y = y1a - 1; y <= y2a + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1a - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2a + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1a - 1; x <= x2a + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1a - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2a + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (y = y1b - 1; y <= y2b + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1b - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2b + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1b - 1; x <= x2b + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1b - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2b + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (y = y1a; y <= y2a; y++) {
        for (x = x1a; x <= x2a; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (y = y1b; y <= y2b; y++) {
        for (x = x1b; x <= x2b; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
        }
    }

    return true;
}

/*!
 * @brief タイプ3の部屋…十字型の部屋を生成する / Type 3 -- Cross shaped rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Builds a room at a row, column coordinate\n
 *\n
 * Room "a" runs north/south, and Room "b" runs east/east\n
 * So the "central pillar" runs from x1a, y1b to x2a, y2b.\n
 *\n
 * Note that currently, the "center" is always 3x3, but I think that\n
 * the code below will work (with "bounds checking") for 5x5, or even\n
 * for unsymetric values like 4x3 or 5x3 or 3x4 or 3x5, or even larger.\n
 */
bool build_type3(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, dy, dx, wy, wx;
    POSITION y1a, x1a, y2a, x2a;
    POSITION y1b, x1b, y2b, x2b;
    POSITION yval, xval;
    bool light;
    grid_type *g_ptr;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    /* Choose lite or dark */
    light = ((floor_ptr->dun_level <= randint1(25)) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));

    /* For now, always 3x3 */
    wx = wy = 1;

    /* Pick max vertical size (at most 4) */
    dy = rand_range(3, 4);

    /* Pick max horizontal size (at most 15) */
    dx = rand_range(3, 11);

    /* Determine extents of the north/south room */
    y1a = yval - dy;
    y2a = yval + dy;
    x1a = xval - wx;
    x2a = xval + wx;

    /* Determine extents of the east/west room */
    y1b = yval - wy;
    y2b = yval + wy;
    x1b = xval - dx;
    x2b = xval + dx;

    /* Place a full floor for room "a" */
    for (y = y1a - 1; y <= y2a + 1; y++) {
        for (x = x1a - 1; x <= x2a + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Place a full floor for room "b" */
    for (y = y1b - 1; y <= y2b + 1; y++) {
        for (x = x1b - 1; x <= x2b + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (y = y1a - 1; y <= y2a + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1a - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2a + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1a - 1; x <= x2a + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1a - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2a + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (y = y1b - 1; y <= y2b + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1b - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2b + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1b - 1; x <= x2b + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1b - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2b + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (y = y1a; y <= y2a; y++) {
        for (x = x1a; x <= x2a; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (y = y1b; y <= y2b; y++) {
        for (x = x1b; x <= x2b; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
        }
    }

    /* Special features (3/4) */
    switch (randint0(4)) {
        /* Large solid middle pillar */
    case 1: {
        for (y = y1b; y <= y2b; y++) {
            for (x = x1a; x <= x2a; x++) {
                g_ptr = &floor_ptr->grid_array[y][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }
        }
        break;
    }

    /* Inner treasure vault */
    case 2: {
        /* Build the vault */
        for (y = y1b; y <= y2b; y++) {
            g_ptr = &floor_ptr->grid_array[y][x1a];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y][x2a];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }
        for (x = x1a; x <= x2a; x++) {
            g_ptr = &floor_ptr->grid_array[y1b][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y2b][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }

        /* Place a secret door on the inner room */
        switch (randint0(4)) {
        case 0:
            place_secret_door(player_ptr, y1b, xval, DOOR_DEFAULT);
            break;
        case 1:
            place_secret_door(player_ptr, y2b, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, yval, x1a, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x2a, DOOR_DEFAULT);
            break;
        }

        /* Place a treasure in the vault */
        place_object(player_ptr, yval, xval, 0L);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, yval, xval, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(player_ptr, yval, xval, 4, 4, randint0(3) + 2);

        break;
    }

    /* Something else */
    case 3: {
        /* Occasionally pinch the center shut */
        if (one_in_(3)) {
            /* Pinch the east/west sides */
            for (y = y1b; y <= y2b; y++) {
                if (y == yval) {
                    continue;
                }
                g_ptr = &floor_ptr->grid_array[y][x1a - 1];
                place_grid(player_ptr, g_ptr, GB_INNER);
                g_ptr = &floor_ptr->grid_array[y][x2a + 1];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }

            /* Pinch the north/south sides */
            for (x = x1a; x <= x2a; x++) {
                if (x == xval) {
                    continue;
                }
                g_ptr = &floor_ptr->grid_array[y1b - 1][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
                g_ptr = &floor_ptr->grid_array[y2b + 1][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }

            /* Sometimes shut using secret doors */
            if (one_in_(3)) {
                int door_type = (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                    ? DOOR_CURTAIN
                                    : (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

                place_secret_door(player_ptr, yval, x1a - 1, door_type);
                place_secret_door(player_ptr, yval, x2a + 1, door_type);
                place_secret_door(player_ptr, y1b - 1, xval, door_type);
                place_secret_door(player_ptr, y2b + 1, xval, door_type);
            }
        }

        /* Occasionally put a "plus" in the center */
        else if (one_in_(3)) {
            g_ptr = &floor_ptr->grid_array[yval][xval];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y1b][xval];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[y2b][xval];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[yval][x1a];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[yval][x2a];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }

        /* Occasionally put a pillar in the center */
        else if (one_in_(3)) {
            g_ptr = &floor_ptr->grid_array[yval][xval];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }

        break;
    }
    }

    return true;
}

/*!
 * @brief タイプ4の部屋…固定サイズの二重構造部屋を生成する / Type 4 -- Large room with inner features
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Possible sub-types:\n
 *	1 - Just an inner room with one door\n
 *	2 - An inner room within an inner room\n
 *	3 - An inner room with pillar(s)\n
 *	4 - Inner room has a maze\n
 *	5 - A set of four inner rooms\n
 */
bool build_type4(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, y1, x1;
    POSITION y2, x2, tmp, yval, xval;
    bool light;
    grid_type *g_ptr;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25)) {
        return false;
    }

    /* Choose lite or dark */
    light = ((floor_ptr->dun_level <= randint1(25)) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));

    /* Large room */
    y1 = yval - 4;
    y2 = yval + 4;
    x1 = xval - 11;
    x2 = xval + 11;

    /* Place a full floor under the room */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->info |= (CAVE_ROOM);
            if (light) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Outer Walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }
    for (x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
    }

    /* The inner room */
    y1 = y1 + 2;
    y2 = y2 - 2;
    x1 = x1 + 2;
    x2 = x2 - 2;

    /* The inner walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        g_ptr = &floor_ptr->grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
    }
    for (x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_INNER);
    }

    /* Inner room variations */
    switch (randint1(5)) {
        /* Just an inner room with a monster */
    case 1: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
            break;
        }

        /* Place a monster in the room */
        vault_monsters(player_ptr, yval, xval, 1);

        break;
    }

    /* Treasure Vault (with a door) */
    case 2: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
            break;
        }

        /* Place another inner room */
        for (y = yval - 1; y <= yval + 1; y++) {
            for (x = xval - 1; x <= xval + 1; x++) {
                if ((x == xval) && (y == yval)) {
                    continue;
                }
                g_ptr = &floor_ptr->grid_array[y][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }
        }

        /* Place a locked door on the inner room */
        switch (randint1(4)) {
        case 1:
            place_locked_door(player_ptr, yval - 1, xval);
            break;
        case 2:
            place_locked_door(player_ptr, yval + 1, xval);
            break;
        case 3:
            place_locked_door(player_ptr, yval, xval - 1);
            break;
        case 4:
            place_locked_door(player_ptr, yval, xval + 1);
            break;
        }

        /* Monsters to guard the "treasure" */
        vault_monsters(player_ptr, yval, xval, randint1(3) + 2);

        /* Object (80%) */
        if (randint0(100) < 80) {
            place_object(player_ptr, yval, xval, 0L);
        }

        /* Stairs (20%) */
        else {
            place_random_stairs(player_ptr, yval, xval);
        }

        /* Traps to protect the treasure */
        vault_traps(player_ptr, yval, xval, 4, 10, 2 + randint1(3));

        break;
    }

    /* Inner pillar(s). */
    case 3: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
            break;
        }

        /* Large Inner Pillar */
        for (y = yval - 1; y <= yval + 1; y++) {
            for (x = xval - 1; x <= xval + 1; x++) {
                g_ptr = &floor_ptr->grid_array[y][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }
        }

        /* Occasionally, two more Large Inner Pillars */
        if (one_in_(2)) {
            tmp = randint1(2);
            for (y = yval - 1; y <= yval + 1; y++) {
                for (x = xval - 5 - tmp; x <= xval - 3 - tmp; x++) {
                    g_ptr = &floor_ptr->grid_array[y][x];
                    place_grid(player_ptr, g_ptr, GB_INNER);
                }
                for (x = xval + 3 + tmp; x <= xval + 5 + tmp; x++) {
                    g_ptr = &floor_ptr->grid_array[y][x];
                    place_grid(player_ptr, g_ptr, GB_INNER);
                }
            }
        }

        /* Occasionally, some Inner rooms */
        if (one_in_(3)) {
            int door_type = (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                ? DOOR_CURTAIN
                                : (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

            /* Long horizontal walls */
            for (x = xval - 5; x <= xval + 5; x++) {
                g_ptr = &floor_ptr->grid_array[yval - 1][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
                g_ptr = &floor_ptr->grid_array[yval + 1][x];
                place_grid(player_ptr, g_ptr, GB_INNER);
            }

            /* Close off the left/right edges */
            g_ptr = &floor_ptr->grid_array[yval][xval - 5];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr = &floor_ptr->grid_array[yval][xval + 5];
            place_grid(player_ptr, g_ptr, GB_INNER);

            /* Secret doors (random top/bottom) */
            place_secret_door(player_ptr, yval - 3 + (randint1(2) * 2), xval - 3, door_type);
            place_secret_door(player_ptr, yval - 3 + (randint1(2) * 2), xval + 3, door_type);

            /* Monsters */
            vault_monsters(player_ptr, yval, xval - 2, randint1(2));
            vault_monsters(player_ptr, yval, xval + 2, randint1(2));

            /* Objects */
            if (one_in_(3)) {
                place_object(player_ptr, yval, xval - 2, 0L);
            }
            if (one_in_(3)) {
                place_object(player_ptr, yval, xval + 2, 0L);
            }
        }

        break;
    }

    /* Maze inside. */
    case 4: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1 - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2 + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1 - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2 + 1, DOOR_DEFAULT);
            break;
        }

        /* Maze (really a checkerboard) */
        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                if (0x1 & (x + y)) {
                    g_ptr = &floor_ptr->grid_array[y][x];
                    place_grid(player_ptr, g_ptr, GB_INNER);
                }
            }
        }

        /* Monsters just love mazes. */
        vault_monsters(player_ptr, yval, xval - 5, randint1(3));
        vault_monsters(player_ptr, yval, xval + 5, randint1(3));

        /* Traps make them entertaining. */
        vault_traps(player_ptr, yval, xval - 3, 2, 8, randint1(3));
        vault_traps(player_ptr, yval, xval + 3, 2, 8, randint1(3));

        /* Mazes should have some treasure too. */
        vault_objects(player_ptr, yval, xval, 3);

        break;
    }

    /* Four small rooms. */
    case 5: {
        int door_type = (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                            ? DOOR_CURTAIN
                            : (dungeons_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

        /* Inner "cross" */
        for (y = y1; y <= y2; y++) {
            g_ptr = &floor_ptr->grid_array[y][xval];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }
        for (x = x1; x <= x2; x++) {
            g_ptr = &floor_ptr->grid_array[yval][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
        }

        /* Doors into the rooms */
        if (randint0(100) < 50) {
            int i = randint1(10);
            place_secret_door(player_ptr, y1 - 1, xval - i, door_type);
            place_secret_door(player_ptr, y1 - 1, xval + i, door_type);
            place_secret_door(player_ptr, y2 + 1, xval - i, door_type);
            place_secret_door(player_ptr, y2 + 1, xval + i, door_type);
        } else {
            int i = randint1(3);
            place_secret_door(player_ptr, yval + i, x1 - 1, door_type);
            place_secret_door(player_ptr, yval - i, x1 - 1, door_type);
            place_secret_door(player_ptr, yval + i, x2 + 1, door_type);
            place_secret_door(player_ptr, yval - i, x2 + 1, door_type);
        }

        /* Treasure, centered at the center of the cross */
        vault_objects(player_ptr, yval, xval, 2 + randint1(2));

        /* Gotta have some monsters. */
        vault_monsters(player_ptr, yval + 1, xval - 4, randint1(4));
        vault_monsters(player_ptr, yval + 1, xval + 4, randint1(4));
        vault_monsters(player_ptr, yval - 1, xval - 4, randint1(4));
        vault_monsters(player_ptr, yval - 1, xval + 4, randint1(4));

        break;
    }
    }

    return true;
}

/*!
 * @brief タイプ11の部屋…円形部屋の生成 / Type 11 -- Build an vertical oval room.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * For every grid in the possible square, check the distance.\n
 * If it's less than the radius, make it a room square.\n
 *\n
 * When done fill from the inside to find the walls,\n
 */
bool build_type11(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION rad, x, y, x0, y0;
    int light = false;

    /* Occasional light */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((randint1(floor_ptr->dun_level) <= 15) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS)) {
        light = true;
    }

    rad = randint0(9);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(player_ptr, dd_ptr, &y0, &x0, rad * 2 + 1, rad * 2 + 1)) {
        return false;
    }

    /* Make circular floor */
    for (x = x0 - rad; x <= x0 + rad; x++) {
        for (y = y0 - rad; y <= y0 + rad; y++) {
            if (distance(y0, x0, y, x) <= rad - 1) {
                /* inside- so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (distance(y0, x0, y, x) <= rad + 1) {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, x0, y0, light, x0 - rad, y0 - rad, x0 + rad, y0 + rad);

    return true;
}

/*!
 * @brief タイプ12の部屋…ドーム型部屋の生成 / Type 12 -- Build crypt room.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * For every grid in the possible square, check the (fake) distance.\n
 * If it's less than the radius, make it a room square.\n
 *\n
 * When done fill from the inside to find the walls,\n
 */
bool build_type12(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION rad, x, y, x0, y0;
    int light = false;
    bool emptyflag = true;

    /* Make a random metric */
    POSITION h1, h2, h3, h4;
    h1 = randint1(32) - 16;
    h2 = randint1(16);
    h3 = randint1(32);
    h4 = randint1(32) - 16;

    /* Occasional light */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((randint1(floor_ptr->dun_level) <= 5) && dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS)) {
        light = true;
    }

    rad = randint1(9);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(player_ptr, dd_ptr, &y0, &x0, rad * 2 + 3, rad * 2 + 3)) {
        return false;
    }

    /* Make floor */
    for (x = x0 - rad; x <= x0 + rad; x++) {
        for (y = y0 - rad; y <= y0 + rad; y++) {
            /* clear room flag */
            floor_ptr->grid_array[y][x].info &= ~(CAVE_ROOM);

            if (dist2(y0, x0, y, x, h1, h2, h3, h4) <= rad - 1) {
                /* inside - so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (distance(y0, x0, y, x) < 3) {
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }

            /* proper boundary for on_defeat_arena_monster */
            if (((y + rad) == y0) || ((y - rad) == y0) || ((x + rad) == x0) || ((x - rad) == x0)) {
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, x0, y0, light, x0 - rad - 1, y0 - rad - 1, x0 + rad + 1, y0 + rad + 1);

    /* Check to see if there is room for an inner vault */
    for (x = x0 - 2; x <= x0 + 2; x++) {
        for (y = y0 - 2; y <= y0 + 2; y++) {
            if (!floor_ptr->grid_array[y][x].is_floor()) {
                /* Wall in the way */
                emptyflag = false;
            }
        }
    }

    if (emptyflag && one_in_(2)) {
        /* Build the vault */
        build_small_room(player_ptr, x0, y0);

        /* Place a treasure in the vault */
        place_object(player_ptr, y0, x0, 0L);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, y0, x0, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(player_ptr, y0, x0, 4, 4, randint0(3) + 2);
    }

    return true;
}
