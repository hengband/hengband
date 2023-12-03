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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    const auto is_curtain = dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 48 : 512);

    /* Pick a room size */
    auto y1 = randint1(4);
    auto x1 = randint1(11);
    auto y2 = randint1(3);
    auto x2 = randint1(11);
    auto xsize = x1 + x2 + 1;
    auto ysize = y1 + y2 + 1;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    int yval;
    int xval;
    auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2);
    if (!is_pos_found) {
        /* Limit to the minimum room size, and retry */
        y1 = 1;
        x1 = 1;
        y2 = 1;
        x2 = 1;

        xsize = x1 + x2 + 1;
        ysize = y1 + y2 + 1;

        /* Find and reserve some space in the dungeon.  Get center of room. */
        is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2);
        if (!is_pos_found) {
            return false;
        }
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor_ptr->dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    y1 = yval - ysize / 2;
    x1 = xval - xsize / 2;
    y2 = yval + (ysize - 1) / 2;
    x2 = xval + (xsize - 1) / 2;

    /* Place a full floor under the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        auto &grid1 = floor_ptr->get_grid({ y, x1 - 1 });
        place_grid(player_ptr, &grid1, GB_OUTER);
        auto &grid2 = floor_ptr->get_grid({ y, x2 + 1 });
        place_grid(player_ptr, &grid2, GB_OUTER);
    }
    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        auto &grid1 = floor_ptr->get_grid({ y1 - 1, x });
        place_grid(player_ptr, &grid1, GB_OUTER);
        auto &grid2 = floor_ptr->get_grid({ y2 + 1, x });
        place_grid(player_ptr, &grid2, GB_OUTER);
    }

    /* Hack -- Occasional curtained room */
    if (is_curtain && (y2 - y1 > 2) && (x2 - x1 > 2)) {
        for (auto y = y1; y <= y2; y++) {
            auto &grid1 = floor_ptr->get_grid({ y, x1 });
            grid1.feat = feat_door[DOOR_CURTAIN].closed;
            grid1.info &= ~(CAVE_MASK);
            auto &grid2 = floor_ptr->get_grid({ y, x2 });
            grid2.feat = feat_door[DOOR_CURTAIN].closed;
            grid2.info &= ~(CAVE_MASK);
        }
        for (auto x = x1; x <= x2; x++) {
            auto &grid1 = floor_ptr->get_grid({ y1, x });
            grid1.feat = feat_door[DOOR_CURTAIN].closed;
            grid1.info &= ~(CAVE_MASK);
            auto &grid2 = floor_ptr->get_grid({ y2, x });
            grid2.feat = feat_door[DOOR_CURTAIN].closed;
            grid2.info &= ~(CAVE_MASK);
        }
    }

    /* Hack -- Occasional pillar room */
    if (one_in_(20)) {
        for (auto y = y1; y <= y2; y += 2) {
            for (auto x = x1; x <= x2; x += 2) {
                auto &grid = floor_ptr->get_grid({ y, x });
                place_grid(player_ptr, &grid, GB_INNER);
            }
        }
    }

    /* Hack -- Occasional room with four pillars */
    else if (one_in_(20)) {
        if ((y1 + 4 < y2) && (x1 + 4 < x2)) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y1 + 1, x1 + 1 }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y1 + 1, x2 - 1 }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y2 - 1, x1 + 1 }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y2 - 1, x2 - 1 }), GB_INNER);
        }
    }

    /* Hack -- Occasional ragged-edge room */
    else if (one_in_(50)) {
        for (auto y = y1 + 2; y <= y2 - 2; y += 2) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x1 }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x2 }), GB_INNER);
        }
        for (auto x = x1 + 2; x <= x2 - 2; x += 2) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y1, x }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y2, x }), GB_INNER);
        }
    }
    /* Hack -- Occasional divided room */
    else if (one_in_(50)) {
        const auto should_close_curtain = (dungeon.flags.has(DungeonFeatureType::CURTAIN)) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 2 : 128);
        if (randint1(100) < 50) {
            /* Horizontal wall */
            for (auto x = x1; x <= x2; x++) {
                place_bold(player_ptr, yval, x, GB_INNER);
                if (should_close_curtain) {
                    floor_ptr->get_grid({ yval, x }).feat = feat_door[DOOR_CURTAIN].closed;
                }
            }

            /* Prevent edge of wall from being tunneled */
            place_bold(player_ptr, yval, x1 - 1, GB_SOLID);
            place_bold(player_ptr, yval, x2 + 1, GB_SOLID);
        } else {
            /* Vertical wall */
            for (auto y = y1; y <= y2; y++) {
                place_bold(player_ptr, y, xval, GB_INNER);
                if (should_close_curtain) {
                    floor_ptr->get_grid({ y, xval }).feat = feat_door[DOOR_CURTAIN].closed;
                }
            }

            /* Prevent edge of wall from being tunneled */
            place_bold(player_ptr, y1 - 1, xval, GB_SOLID);
            place_bold(player_ptr, y2 + 1, xval, GB_SOLID);
        }

        place_random_door(player_ptr, yval, xval, true);
        if (should_close_curtain) {
            floor_ptr->get_grid({ yval, xval }).feat = feat_door[DOOR_CURTAIN].closed;
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
    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int yval;
    int xval;
    const auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25);
    if (!is_pos_found) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = (floor_ptr->dun_level <= randint1(25)) && floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);

    /* Determine extents of the first room */
    auto y1a = yval - randint1(4);
    auto y2a = yval + randint1(3);
    auto x1a = xval - randint1(11);
    auto x2a = xval + randint1(10);

    /* Determine extents of the second room */
    auto y1b = yval - randint1(3);
    auto y2b = yval + randint1(4);
    auto x1b = xval - randint1(10);
    auto x2b = xval + randint1(11);

    /* Place a full floor for room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        for (auto x = x1a - 1; x <= x2a + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place a full floor for room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        for (auto x = x1b - 1; x <= x2b + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1a - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2a + 1 }), GB_OUTER);
    }
    for (auto x = x1a - 1; x <= x2a + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1a - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2a + 1, x }), GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1b - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2b + 1 }), GB_OUTER);
    }
    for (auto x = x1b - 1; x <= x2b + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1b - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2b + 1, x }), GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (auto y = y1a; y <= y2a; y++) {
        for (auto x = x1a; x <= x2a; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (auto y = y1b; y <= y2b; y++) {
        for (auto x = x1b; x <= x2b; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_FLOOR);
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
    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int yval;
    int xval;
    const auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25);
    if (!is_pos_found) {
        return false;
    }

    /* Choose lite or dark */
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    const auto should_brighten = ((floor_ptr->dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* For now, always 3x3 */
    auto wx = 1;
    auto wy = 1;

    /* Pick max vertical size (at most 4) */
    auto dy = rand_range(3, 4);

    /* Pick max horizontal size (at most 15) */
    auto dx = rand_range(3, 11);

    /* Determine extents of the north/south room */
    auto y1a = yval - dy;
    auto y2a = yval + dy;
    auto x1a = xval - wx;
    auto x2a = xval + wx;

    /* Determine extents of the east/west room */
    auto y1b = yval - wy;
    auto y2b = yval + wy;
    auto x1b = xval - dx;
    auto x2b = xval + dx;

    /* Place a full floor for room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        for (auto x = x1a - 1; x <= x2a + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place a full floor for room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        for (auto x = x1b - 1; x <= x2b + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1a - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2a + 1 }), GB_OUTER);
    }
    for (auto x = x1a - 1; x <= x2a + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1a - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2a + 1, x }), GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1b - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2b + 1 }), GB_OUTER);
    }
    for (auto x = x1b - 1; x <= x2b + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1b - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2b + 1, x }), GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (auto y = y1a; y <= y2a; y++) {
        for (auto x = x1a; x <= x2a; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (auto y = y1b; y <= y2b; y++) {
        for (auto x = x1b; x <= x2b; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_FLOOR);
        }
    }

    /* Special features (3/4) */
    switch (randint0(4)) {
        /* Large solid middle pillar */
    case 1: {
        for (auto y = y1b; y <= y2b; y++) {
            for (auto x = x1a; x <= x2a; x++) {
                place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
            }
        }
        break;
    }

    /* Inner treasure vault */
    case 2: {
        /* Build the vault */
        for (auto y = y1b; y <= y2b; y++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x1a }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y, x2a }), GB_INNER);
        }
        for (auto x = x1a; x <= x2a; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y1b, x }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y1b, x }), GB_INNER);
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
        place_object(player_ptr, yval, xval, 0);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, yval, xval, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(floor_ptr, yval, xval, 4, 4, randint0(3) + 2);

        break;
    }

    /* Something else */
    case 3: {
        /* Occasionally pinch the center shut */
        if (one_in_(3)) {
            /* Pinch the east/west sides */
            for (auto y = y1b; y <= y2b; y++) {
                if (y == yval) {
                    continue;
                }

                place_grid(player_ptr, &floor_ptr->get_grid({ y, x1a - 1 }), GB_INNER);
                place_grid(player_ptr, &floor_ptr->get_grid({ y, x2a + 1 }), GB_INNER);
            }

            /* Pinch the north/south sides */
            for (auto x = x1a; x <= x2a; x++) {
                if (x == xval) {
                    continue;
                }

                place_grid(player_ptr, &floor_ptr->grid_array[y1b - 1][x], GB_INNER);
                place_grid(player_ptr, &floor_ptr->grid_array[y2b + 1][x], GB_INNER);
            }

            /* Sometimes shut using secret doors */
            if (one_in_(3)) {
                int door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                    ? DOOR_CURTAIN
                                    : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

                place_secret_door(player_ptr, yval, x1a - 1, door_type);
                place_secret_door(player_ptr, yval, x2a + 1, door_type);
                place_secret_door(player_ptr, y1b - 1, xval, door_type);
                place_secret_door(player_ptr, y2b + 1, xval, door_type);
            }
        }

        /* Occasionally put a "plus" in the center */
        else if (one_in_(3)) {
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, xval }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y1b, xval }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ y2b, xval }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, x1a }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, x2a }), GB_INNER);
        }

        /* Occasionally put a pillar in the center */
        else if (one_in_(3)) {
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, xval }), GB_INNER);
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
    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    int yval;
    int xval;
    const auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, 11, 25);
    if (!is_pos_found) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor_ptr->dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* Large room */
    const auto y1_outer = yval - 4;
    const auto y2_outer = yval + 4;
    const auto x1_outer = xval - 11;
    const auto x2_outer = xval + 11;

    /* Place a full floor under the room */
    for (auto y = y1_outer - 1; y <= y2_outer + 1; y++) {
        for (auto x = x1_outer - 1; x <= x2_outer + 1; x++) {
            auto &grid = floor_ptr->get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Outer Walls */
    for (auto y = y1_outer - 1; y <= y2_outer + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1_outer - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2_outer + 1 }), GB_OUTER);
    }
    for (auto x = x1_outer - 1; x <= x2_outer + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1_outer - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2_outer + 1, x }), GB_OUTER);
    }

    const auto y1_inner = y1_outer + 2;
    const auto y2_inner = y2_outer - 2;
    const auto x1_inner = x1_outer + 2;
    const auto x2_inner = x2_outer - 2;

    /* The inner walls */
    for (auto y = y1_inner - 1; y <= y2_inner + 1; y++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x1_inner - 1 }), GB_INNER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y, x2_inner + 1 }), GB_INNER);
    }
    for (auto x = x1_inner - 1; x <= x2_inner + 1; x++) {
        place_grid(player_ptr, &floor_ptr->get_grid({ y1_inner - 1, x }), GB_INNER);
        place_grid(player_ptr, &floor_ptr->get_grid({ y2_inner + 1, x }), GB_INNER);
    }

    /* Inner room variations */
    switch (randint1(5)) {
        /* Just an inner room with a monster */
    case 1: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1_inner - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2_inner + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1_inner - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2_inner + 1, DOOR_DEFAULT);
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
            place_secret_door(player_ptr, y1_inner - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2_inner + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1_inner - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2_inner + 1, DOOR_DEFAULT);
            break;
        }

        /* Place another inner room */
        for (auto y = yval - 1; y <= yval + 1; y++) {
            for (auto x = xval - 1; x <= xval + 1; x++) {
                if ((x == xval) && (y == yval)) {
                    continue;
                }

                place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
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
        vault_traps(floor_ptr, yval, xval, 4, 10, 2 + randint1(3));

        break;
    }

    /* Inner pillar(s). */
    case 3: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, y1_inner - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2_inner + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1_inner - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2_inner + 1, DOOR_DEFAULT);
            break;
        }

        /* Large Inner Pillar */
        for (auto y = yval - 1; y <= yval + 1; y++) {
            for (auto x = xval - 1; x <= xval + 1; x++) {
                place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
            }
        }

        /* Occasionally, two more Large Inner Pillars */
        if (one_in_(2)) {
            const auto tmp = randint1(2);
            for (auto y = yval - 1; y <= yval + 1; y++) {
                for (auto x = xval - 5 - tmp; x <= xval - 3 - tmp; x++) {
                    place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
                }

                for (auto x = xval + 3 + tmp; x <= xval + 5 + tmp; x++) {
                    place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
                }
            }
        }

        /* Occasionally, some Inner rooms */
        if (one_in_(3)) {
            int door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                ? DOOR_CURTAIN
                                : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

            /* Long horizontal walls */
            for (auto x = xval - 5; x <= xval + 5; x++) {
                place_grid(player_ptr, &floor_ptr->get_grid({ yval - 1, x }), GB_INNER);
                place_grid(player_ptr, &floor_ptr->get_grid({ yval + 1, x }), GB_INNER);
            }

            /* Close off the left/right edges */
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, xval - 5 }), GB_INNER);
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, xval + 5 }), GB_INNER);

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
            place_secret_door(player_ptr, y1_inner - 1, xval, DOOR_DEFAULT);
            break;
        case 2:
            place_secret_door(player_ptr, y2_inner + 1, xval, DOOR_DEFAULT);
            break;
        case 3:
            place_secret_door(player_ptr, yval, x1_inner - 1, DOOR_DEFAULT);
            break;
        case 4:
            place_secret_door(player_ptr, yval, x2_inner + 1, DOOR_DEFAULT);
            break;
        }

        /* Maze (really a checkerboard) */
        for (auto y = y1_inner; y <= y2_inner; y++) {
            for (auto x = x1_inner; x <= x2_inner; x++) {
                if (0x1 & (x + y)) {
                    place_grid(player_ptr, &floor_ptr->get_grid({ y, x }), GB_INNER);
                }
            }
        }

        /* Monsters just love mazes. */
        vault_monsters(player_ptr, yval, xval - 5, randint1(3));
        vault_monsters(player_ptr, yval, xval + 5, randint1(3));

        /* Traps make them entertaining. */
        vault_traps(floor_ptr, yval, xval - 3, 2, 8, randint1(3));
        vault_traps(floor_ptr, yval, xval + 3, 2, 8, randint1(3));

        /* Mazes should have some treasure too. */
        vault_objects(player_ptr, yval, xval, 3);

        break;
    }

    /* Four small rooms. */
    case 5: {
        int door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                            ? DOOR_CURTAIN
                            : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

        /* Inner "cross" */
        for (auto y = y1_inner; y <= y2_inner; y++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ y, xval }), GB_INNER);
        }

        for (auto x = x1_inner; x <= x2_inner; x++) {
            place_grid(player_ptr, &floor_ptr->get_grid({ yval, x }), GB_INNER);
        }

        /* Doors into the rooms */
        if (randint0(100) < 50) {
            int i = randint1(10);
            place_secret_door(player_ptr, y1_inner - 1, xval - i, door_type);
            place_secret_door(player_ptr, y1_inner - 1, xval + i, door_type);
            place_secret_door(player_ptr, y2_inner + 1, xval - i, door_type);
            place_secret_door(player_ptr, y2_inner + 1, xval + i, door_type);
        } else {
            int i = randint1(3);
            place_secret_door(player_ptr, yval + i, x1_inner - 1, door_type);
            place_secret_door(player_ptr, yval - i, x1_inner - 1, door_type);
            place_secret_door(player_ptr, yval + i, x2_inner + 1, door_type);
            place_secret_door(player_ptr, yval - i, x2_inner + 1, door_type);
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
    /* Occasional light */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto should_brighten = (randint1(floor_ptr->dun_level) <= 15) && floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);
    const auto rad = randint0(9);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    int yval;
    int xval;
    const auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, rad * 2 + 1, rad * 2 + 1);
    if (!is_pos_found) {
        return false;
    }

    /* Make circular floor */
    for (auto x = xval - rad; x <= xval + rad; x++) {
        for (auto y = yval - rad; y <= yval + rad; y++) {
            if (distance(yval, xval, y, x) <= rad - 1) {
                /* inside- so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (distance(yval, xval, y, x) <= rad + 1) {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, xval, yval, should_brighten, xval - rad, yval - rad, xval + rad, yval + rad);
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
    /* Make a random metric */
    const auto h1 = randint1(32) - 16;
    const auto h2 = randint1(16);
    const auto h3 = randint1(32);
    const auto h4 = randint1(32) - 16;

    /* Occasional light */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto should_brighten = (randint1(floor_ptr->dun_level) <= 5) && floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);
    const auto rad = randint1(9);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    int yval;
    int xval;
    const auto is_pos_found = find_space(player_ptr, dd_ptr, &yval, &xval, rad * 2 + 3, rad * 2 + 3);
    if (!is_pos_found) {
        return false;
    }

    /* Make floor */
    for (auto x = xval - rad; x <= xval + rad; x++) {
        for (auto y = yval - rad; y <= yval + rad; y++) {
            /* clear room flag */
            floor_ptr->grid_array[y][x].info &= ~(CAVE_ROOM);

            if (dist2(yval, xval, y, x, h1, h2, h3, h4) <= rad - 1) {
                /* inside - so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (distance(yval, xval, y, x) < 3) {
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }

            /* proper boundary for on_defeat_arena_monster */
            if (((y + rad) == yval) || ((y - rad) == yval) || ((x + rad) == xval) || ((x - rad) == xval)) {
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, xval, yval, should_brighten, xval - rad - 1, yval - rad - 1, xval + rad + 1, yval + rad + 1);

    /* Check to see if there is room for an inner vault */
    auto is_empty = true;
    for (auto x = xval - 2; x <= xval + 2; x++) {
        if (!is_empty) {
            break;
        }

        for (auto y = yval - 2; y <= yval + 2; y++) {
            if (!floor_ptr->get_grid({ y, x }).is_floor()) {
                is_empty = false;
                break;
            }
        }
    }

    if (is_empty && one_in_(2)) {
        /* Build the vault */
        build_small_room(player_ptr, xval, yval);

        /* Place a treasure in the vault */
        place_object(player_ptr, yval, xval, 0);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, yval, xval, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(floor_ptr, yval, xval, 4, 4, randint0(3) + 2);
    }

    return true;
}
