#include "room/rooms-normal.h"
#include "dungeon/dungeon-flag-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "room/door-definition.h"
#include "room/rooms-builder.h"
#include "room/space-finder.h"
#include "room/vault-builder.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief タイプ1の部屋…通常可変長方形の部屋を生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 部屋の配置スペースを確保できたか否か
 */
bool build_type1(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    const auto is_curtain = dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 48 : 512);

    /* Pick a room size */
    auto height = randint1(4) + randint1(3) + 1;
    auto width = randint1(11) + randint1(11) + 1;

    auto center = find_space(player_ptr, dd_ptr, height + 2, width + 2);
    if (!center) {
        /* Limit to the minimum room size, and retry */
        width = 3;
        height = 3;

        /* Find and reserve some space in the dungeon.  Get center of room. */
        center = find_space(player_ptr, dd_ptr, height + 2, width + 2);
        if (!center) {
            return false;
        }
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor.dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    const auto top = center->y - height / 2;
    const auto left = center->x - width / 2;
    const auto bottom = center->y + (height - 1) / 2;
    const auto right = center->x + (width - 1) / 2;

    /* Place a full floor under the room */
    for (auto y = top - 1; y <= bottom + 1; y++) {
        for (auto x = left - 1; x <= right + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (auto y = top - 1; y <= bottom + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, left - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, right + 1 }), GB_OUTER);
    }

    for (auto x = left - 1; x <= right + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ top - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ bottom + 1, x }), GB_OUTER);
    }

    /* Hack -- Occasional curtained room */
    if (is_curtain && (bottom - top > 2) && (right - left > 2)) {
        for (auto y = top; y <= bottom; y++) {
            floor.get_grid({ y, left }).place_closed_curtain();
            floor.get_grid({ y, right }).place_closed_curtain();
        }

        for (auto x = left; x <= right; x++) {
            floor.get_grid({ top, x }).place_closed_curtain();
            floor.get_grid({ bottom, x }).place_closed_curtain();
        }
    }

    /* Hack -- Occasional pillar room */
    if (one_in_(20)) {
        for (auto y = top; y <= bottom; y += 2) {
            for (auto x = left; x <= right; x += 2) {
                auto &grid = floor.get_grid({ y, x });
                place_grid(player_ptr, &grid, GB_INNER);
            }
        }

        return true;
    }

    /* Hack -- Occasional room with four pillars */
    if (one_in_(20)) {
        if ((top + 4 < bottom) && (left + 4 < right)) {
            place_grid(player_ptr, &floor.get_grid({ top + 1, left + 1 }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ top + 1, right - 1 }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ bottom - 1, left + 1 }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ bottom - 1, right - 1 }), GB_INNER);
        }

        return true;
    }

    /* Hack -- Occasional ragged-edge room */
    if (one_in_(50)) {
        for (auto y = top + 2; y <= bottom - 2; y += 2) {
            place_grid(player_ptr, &floor.get_grid({ y, left }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ y, right }), GB_INNER);
        }

        for (auto x = left + 2; x <= right - 2; x += 2) {
            place_grid(player_ptr, &floor.get_grid({ top, x }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ bottom, x }), GB_INNER);
        }

        return true;
    }

    if (!one_in_(50)) {
        return true;
    }

    /* Hack -- Occasional divided room */
    const auto should_close_curtain = (dungeon.flags.has(DungeonFeatureType::CURTAIN)) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 2 : 128);
    const auto &doors = Doors::get_instance();
    if (randint1(100) < 50) {
        /* Horizontal wall */
        for (auto x = left; x <= right; x++) {
            place_bold(player_ptr, center->y, x, GB_INNER);
            if (should_close_curtain) {
                floor.set_terrain_id_at({ center->y, x }, doors.get_door(DoorKind::CURTAIN).closed);
            }
        }

        /* Prevent edge of wall from being tunneled */
        place_bold(player_ptr, center->y, left - 1, GB_SOLID);
        place_bold(player_ptr, center->y, right + 1, GB_SOLID);
    } else {
        /* Vertical wall */
        for (auto y = top; y <= bottom; y++) {
            place_bold(player_ptr, y, center->x, GB_INNER);
            if (should_close_curtain) {
                floor.set_terrain_id_at({ y, center->x }, doors.get_door(DoorKind::CURTAIN).closed);
            }
        }

        /* Prevent edge of wall from being tunneled */
        place_bold(player_ptr, top - 1, center->x, GB_SOLID);
        place_bold(player_ptr, bottom + 1, center->x, GB_SOLID);
    }

    place_random_door(player_ptr, *center, true);
    if (should_close_curtain) {
        floor.set_terrain_id_at(*center, doors.get_door(DoorKind::CURTAIN).closed);
    }

    return true;
}

/*!
 * @brief タイプ2の部屋…二重長方形の部屋を生成する / Type 2 -- Overlapping rectangular rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type2(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto center = find_space(player_ptr, dd_ptr, 11, 25);
    if (!center) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = (floor.dun_level <= randint1(25)) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);

    /* Determine extents of the first room */
    auto y1a = center->y - randint1(4);
    auto y2a = center->y + randint1(3);
    auto x1a = center->x - randint1(11);
    auto x2a = center->x + randint1(10);

    /* Determine extents of the second room */
    auto y1b = center->y - randint1(3);
    auto y2b = center->y + randint1(4);
    auto x1b = center->x - randint1(10);
    auto x2b = center->x + randint1(11);

    /* Place a full floor for room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        for (auto x = x1a - 1; x <= x2a + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
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
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1a - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2a + 1 }), GB_OUTER);
    }
    for (auto x = x1a - 1; x <= x2a + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1a - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2a + 1, x }), GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1b - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2b + 1 }), GB_OUTER);
    }
    for (auto x = x1b - 1; x <= x2b + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1b - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2b + 1, x }), GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (auto y = y1a; y <= y2a; y++) {
        for (auto x = x1a; x <= x2a; x++) {
            place_grid(player_ptr, &floor.get_grid({ y, x }), GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (auto y = y1b; y <= y2b; y++) {
        for (auto x = x1b; x <= x2b; x++) {
            place_grid(player_ptr, &floor.get_grid({ y, x }), GB_FLOOR);
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
bool build_type3(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto center = find_space(player_ptr, dd_ptr, 11, 25);
    if (!center) {
        return false;
    }

    /* Choose lite or dark */
    const auto &dungeon = floor.get_dungeon_definition();
    const auto should_brighten = ((floor.dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* For now, always 3x3 */
    auto wx = 1;
    auto wy = 1;

    /* Pick max vertical size (at most 4) */
    auto dy = rand_range(3, 4);

    /* Pick max horizontal size (at most 15) */
    auto dx = rand_range(3, 11);

    /* Determine extents of the north/south room */
    auto y1a = center->y - dy;
    auto y2a = center->y + dy;
    auto x1a = center->x - wx;
    auto x2a = center->x + wx;

    /* Determine extents of the east/west room */
    auto y1b = center->y - wy;
    auto y2b = center->y + wy;
    auto x1b = center->x - dx;
    auto x2b = center->x + dx;

    /* Place a full floor for room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        for (auto x = x1a - 1; x <= x2a + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
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
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Place the walls around room "a" */
    for (auto y = y1a - 1; y <= y2a + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1a - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2a + 1 }), GB_OUTER);
    }
    for (auto x = x1a - 1; x <= x2a + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1a - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2a + 1, x }), GB_OUTER);
    }

    /* Place the walls around room "b" */
    for (auto y = y1b - 1; y <= y2b + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1b - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2b + 1 }), GB_OUTER);
    }
    for (auto x = x1b - 1; x <= x2b + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1b - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2b + 1, x }), GB_OUTER);
    }

    /* Replace the floor for room "a" */
    for (auto y = y1a; y <= y2a; y++) {
        for (auto x = x1a; x <= x2a; x++) {
            place_grid(player_ptr, &floor.get_grid({ y, x }), GB_FLOOR);
        }
    }

    /* Replace the floor for room "b" */
    for (auto y = y1b; y <= y2b; y++) {
        for (auto x = x1b; x <= x2b; x++) {
            place_grid(player_ptr, &floor.get_grid({ y, x }), GB_FLOOR);
        }
    }

    const Rect2D rect_inner(y1b, x1a, y2b, x2a);
    /* Special features (3/4) */
    switch (randint0(4)) {
        /* Large solid middle pillar */
    case 1: {
        rect_inner.each_area([&](const auto &pos) {
            place_grid(player_ptr, &floor.get_grid(pos), GB_INNER);
        });
        break;
    }

    /* Inner treasure vault */
    case 2: {
        /* Build the vault */
        rect_inner.each_edge([&](const auto &pos) {
            place_grid(player_ptr, &floor.get_grid(pos), GB_INNER);
        });

        /* Place a secret door on the inner room */
        switch (randint0(4)) {
        case 0:
            place_secret_door(player_ptr, { y1b, center->x });
            break;
        case 1:
            place_secret_door(player_ptr, { y2b, center->x });
            break;
        case 2:
            place_secret_door(player_ptr, { center->y, x1a });
            break;
        case 3:
            place_secret_door(player_ptr, { center->y, x2a });
            break;
        }

        /* Place a treasure in the vault */
        place_object(player_ptr, center->y, center->x, 0);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, center->y, center->x, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(floor, center->y, center->x, 4, 4, randint0(3) + 2);

        break;
    }

    /* Something else */
    case 3: {
        /* Occasionally pinch the center shut */
        if (one_in_(3)) {
            /* Pinch the east/west sides */
            for (auto y = y1b; y <= y2b; y++) {
                if (y == center->y) {
                    continue;
                }

                place_grid(player_ptr, &floor.get_grid({ y, x1a - 1 }), GB_INNER);
                place_grid(player_ptr, &floor.get_grid({ y, x2a + 1 }), GB_INNER);
            }

            /* Pinch the north/south sides */
            for (auto x = x1a; x <= x2a; x++) {
                if (x == center->x) {
                    continue;
                }

                place_grid(player_ptr, &floor.grid_array[y1b - 1][x], GB_INNER);
                place_grid(player_ptr, &floor.grid_array[y2b + 1][x], GB_INNER);
            }

            /* Sometimes shut using secret doors */
            if (one_in_(3)) {
                const auto door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                           ? DoorKind::CURTAIN
                                           : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DoorKind::GLASS_DOOR : DoorKind::DOOR);

                place_secret_door(player_ptr, { center->y, x1a - 1 }, door_type);
                place_secret_door(player_ptr, { center->y, x2a + 1 }, door_type);
                place_secret_door(player_ptr, { y1b - 1, center->x }, door_type);
                place_secret_door(player_ptr, { y2b + 1, center->x }, door_type);
            }
        }

        /* Occasionally put a "plus" in the center */
        else if (one_in_(3)) {
            place_grid(player_ptr, &floor.get_grid(*center), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ y1b, center->x }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ y2b, center->x }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ center->y, x1a }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ center->y, x2a }), GB_INNER);
        }

        /* Occasionally put a pillar in the center */
        else if (one_in_(3)) {
            place_grid(player_ptr, &floor.get_grid(*center), GB_INNER);
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
bool build_type4(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    const auto center = find_space(player_ptr, dd_ptr, 11, 25);
    if (!center) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor.dun_level <= randint1(25)) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS));

    /* Large room */
    const auto y1_outer = center->y - 4;
    const auto y2_outer = center->y + 4;
    const auto x1_outer = center->x - 11;
    const auto x2_outer = center->x + 11;

    /* Place a full floor under the room */
    for (auto y = y1_outer - 1; y <= y2_outer + 1; y++) {
        for (auto x = x1_outer - 1; x <= x2_outer + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Outer Walls */
    for (auto y = y1_outer - 1; y <= y2_outer + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1_outer - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2_outer + 1 }), GB_OUTER);
    }
    for (auto x = x1_outer - 1; x <= x2_outer + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1_outer - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2_outer + 1, x }), GB_OUTER);
    }

    const auto y1_inner = y1_outer + 2;
    const auto y2_inner = y2_outer - 2;
    const auto x1_inner = x1_outer + 2;
    const auto x2_inner = x2_outer - 2;

    /* The inner walls */
    for (auto y = y1_inner - 1; y <= y2_inner + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1_inner - 1 }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ y, x2_inner + 1 }), GB_INNER);
    }
    for (auto x = x1_inner - 1; x <= x2_inner + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1_inner - 1, x }), GB_INNER);
        place_grid(player_ptr, &floor.get_grid({ y2_inner + 1, x }), GB_INNER);
    }

    /* Inner room variations */
    switch (randint1(5)) {
        /* Just an inner room with a monster */
    case 1: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, { y1_inner - 1, center->x });
            break;
        case 2:
            place_secret_door(player_ptr, { y2_inner + 1, center->x });
            break;
        case 3:
            place_secret_door(player_ptr, { center->y, x1_inner - 1 });
            break;
        case 4:
            place_secret_door(player_ptr, { center->y, x2_inner + 1 });
            break;
        }

        /* Place a monster in the room */
        vault_monsters(player_ptr, center->y, center->x, 1);

        break;
    }

    /* Treasure Vault (with a door) */
    case 2: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, { y1_inner - 1, center->x });
            break;
        case 2:
            place_secret_door(player_ptr, { y2_inner + 1, center->x });
            break;
        case 3:
            place_secret_door(player_ptr, { center->y, x1_inner - 1 });
            break;
        case 4:
            place_secret_door(player_ptr, { center->y, x2_inner + 1 });
            break;
        }

        /* Place another inner room */
        for (auto y = center->y - 1; y <= center->y + 1; y++) {
            for (auto x = center->x - 1; x <= center->x + 1; x++) {
                if ((x == center->x) && (y == center->y)) {
                    continue;
                }

                place_grid(player_ptr, &floor.get_grid({ y, x }), GB_INNER);
            }
        }

        /* Place a locked door on the inner room */
        switch (randint1(4)) {
        case 1:
            place_locked_door(player_ptr, *center + Pos2DVec(-1, 0));
            break;
        case 2:
            place_locked_door(player_ptr, *center + Pos2DVec(1, 0));
            break;
        case 3:
            place_locked_door(player_ptr, *center + Pos2DVec(0, -1));
            break;
        case 4:
            place_locked_door(player_ptr, *center + Pos2DVec(0, 1));
            break;
        }

        /* Monsters to guard the "treasure" */
        vault_monsters(player_ptr, center->y, center->x, randint1(3) + 2);

        if (evaluate_percent(80)) {
            place_object(player_ptr, center->y, center->x, 0L);
        } else {
            player_ptr->current_floor_ptr->place_random_stairs(*center);
        }

        /* Traps to protect the treasure */
        vault_traps(floor, center->y, center->x, 4, 10, 2 + randint1(3));

        break;
    }

    /* Inner pillar(s). */
    case 3: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, { y1_inner - 1, center->x });
            break;
        case 2:
            place_secret_door(player_ptr, { y2_inner + 1, center->x });
            break;
        case 3:
            place_secret_door(player_ptr, { center->y, x1_inner - 1 });
            break;
        case 4:
            place_secret_door(player_ptr, { center->y, x2_inner + 1 });
            break;
        }

        /* Large Inner Pillar */
        for (auto y = center->y - 1; y <= center->y + 1; y++) {
            for (auto x = center->x - 1; x <= center->x + 1; x++) {
                place_grid(player_ptr, &floor.get_grid({ y, x }), GB_INNER);
            }
        }

        /* Occasionally, two more Large Inner Pillars */
        if (one_in_(2)) {
            const auto tmp = randint1(2);
            for (auto y = center->y - 1; y <= center->y + 1; y++) {
                for (auto x = center->x - 5 - tmp; x <= center->x - 3 - tmp; x++) {
                    place_grid(player_ptr, &floor.get_grid({ y, x }), GB_INNER);
                }

                for (auto x = center->x + 3 + tmp; x <= center->x + 5 + tmp; x++) {
                    place_grid(player_ptr, &floor.get_grid({ y, x }), GB_INNER);
                }
            }
        }

        /* Occasionally, some Inner rooms */
        if (one_in_(3)) {
            const auto door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                       ? DoorKind::CURTAIN
                                       : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DoorKind::GLASS_DOOR : DoorKind::DOOR);

            /* Long horizontal walls */
            for (auto x = center->x - 5; x <= center->x + 5; x++) {
                place_grid(player_ptr, &floor.get_grid({ center->y - 1, x }), GB_INNER);
                place_grid(player_ptr, &floor.get_grid({ center->y + 1, x }), GB_INNER);
            }

            /* Close off the left/right edges */
            place_grid(player_ptr, &floor.get_grid({ center->y, center->x - 5 }), GB_INNER);
            place_grid(player_ptr, &floor.get_grid({ center->y, center->x + 5 }), GB_INNER);

            /* Secret doors (random top/bottom) */
            place_secret_door(player_ptr, { center->y - 3 + (randint1(2) * 2), center->x - 3 }, door_type);
            place_secret_door(player_ptr, { center->y - 3 + (randint1(2) * 2), center->x + 3 }, door_type);

            /* Monsters */
            vault_monsters(player_ptr, center->y, center->x - 2, randint1(2));
            vault_monsters(player_ptr, center->y, center->x + 2, randint1(2));

            /* Objects */
            if (one_in_(3)) {
                place_object(player_ptr, center->y, center->x - 2, 0L);
            }
            if (one_in_(3)) {
                place_object(player_ptr, center->y, center->x + 2, 0L);
            }
        }

        break;
    }

    /* Maze inside. */
    case 4: {
        /* Place a secret door */
        switch (randint1(4)) {
        case 1:
            place_secret_door(player_ptr, { y1_inner - 1, center->x });
            break;
        case 2:
            place_secret_door(player_ptr, { y2_inner + 1, center->x });
            break;
        case 3:
            place_secret_door(player_ptr, { center->y, x1_inner - 1 });
            break;
        case 4:
            place_secret_door(player_ptr, { center->y, x2_inner + 1 });
            break;
        }

        /* Maze (really a checkerboard) */
        for (auto y = y1_inner; y <= y2_inner; y++) {
            for (auto x = x1_inner; x <= x2_inner; x++) {
                if (0x1 & (x + y)) {
                    place_grid(player_ptr, &floor.get_grid({ y, x }), GB_INNER);
                }
            }
        }

        /* Monsters just love mazes. */
        vault_monsters(player_ptr, center->y, center->x - 5, randint1(3));
        vault_monsters(player_ptr, center->y, center->x + 5, randint1(3));

        /* Traps make them entertaining. */
        vault_traps(floor, center->y, center->x - 3, 2, 8, randint1(3));
        vault_traps(floor, center->y, center->x + 3, 2, 8, randint1(3));

        /* Mazes should have some treasure too. */
        vault_objects(player_ptr, center->y, center->x, 3);

        break;
    }

    /* Four small rooms. */
    case 5: {
        const auto door_type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                                   ? DoorKind::CURTAIN
                                   : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DoorKind::GLASS_DOOR : DoorKind::DOOR);

        /* Inner "cross" */
        for (auto y = y1_inner; y <= y2_inner; y++) {
            place_grid(player_ptr, &floor.get_grid({ y, center->x }), GB_INNER);
        }

        for (auto x = x1_inner; x <= x2_inner; x++) {
            place_grid(player_ptr, &floor.get_grid({ center->y, x }), GB_INNER);
        }

        /* Doors into the rooms */
        if (one_in_(2)) {
            int i = randint1(10);
            place_secret_door(player_ptr, { y1_inner - 1, center->x - i }, door_type);
            place_secret_door(player_ptr, { y1_inner - 1, center->x + i }, door_type);
            place_secret_door(player_ptr, { y2_inner + 1, center->x - i }, door_type);
            place_secret_door(player_ptr, { y2_inner + 1, center->x + i }, door_type);
        } else {
            int i = randint1(3);
            place_secret_door(player_ptr, { center->y + i, x1_inner - 1 }, door_type);
            place_secret_door(player_ptr, { center->y - i, x1_inner - 1 }, door_type);
            place_secret_door(player_ptr, { center->y + i, x2_inner + 1 }, door_type);
            place_secret_door(player_ptr, { center->y - i, x2_inner + 1 }, door_type);
        }

        /* Treasure, centered at the center of the cross */
        vault_objects(player_ptr, center->y, center->x, 2 + randint1(2));

        /* Gotta have some monsters. */
        vault_monsters(player_ptr, center->y + 1, center->x - 4, randint1(4));
        vault_monsters(player_ptr, center->y + 1, center->x + 4, randint1(4));
        vault_monsters(player_ptr, center->y - 1, center->x - 4, randint1(4));
        vault_monsters(player_ptr, center->y - 1, center->x + 4, randint1(4));

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
bool build_type11(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* Occasional light */
    auto &floor = *player_ptr->current_floor_ptr;
    const auto should_brighten = (randint1(floor.dun_level) <= 15) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);
    const auto rad = randint0(9);
    const auto center = find_space(player_ptr, dd_ptr, rad * 2 + 1, rad * 2 + 1);
    if (!center) {
        return false;
    }

    /* Make circular floor */
    for (auto x = center->x - rad; x <= center->x + rad; x++) {
        for (auto y = center->y - rad; y <= center->y + rad; y++) {
            if (Grid::calc_distance(*center, { y, x }) <= rad - 1) {
                /* inside- so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (Grid::calc_distance(*center, { y, x }) <= rad + 1) {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, center->x, center->y, should_brighten, center->x - rad, center->y - rad, center->x + rad, center->y + rad);
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
bool build_type12(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* Make a random metric */
    const auto h1 = randint1(32) - 16;
    const auto h2 = randint1(16);
    const auto h3 = randint1(32);
    const auto h4 = randint1(32) - 16;

    /* Occasional light */
    auto &floor = *player_ptr->current_floor_ptr;
    const auto should_brighten = (randint1(floor.dun_level) <= 5) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS);
    const auto rad = randint1(9);

    const auto center = find_space(player_ptr, dd_ptr, rad * 2 + 3, rad * 2 + 3);
    if (!center) {
        return false;
    }

    /* Make floor */
    for (auto x = center->x - rad; x <= center->x + rad; x++) {
        for (auto y = center->y - rad; y <= center->y + rad; y++) {
            /* clear room flag */
            floor.grid_array[y][x].info &= ~(CAVE_ROOM);

            if (dist2(center->y, center->x, y, x, h1, h2, h3, h4) <= rad - 1) {
                /* inside - so is floor */
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else if (Grid::calc_distance(*center, { y, x }) < 3) {
                place_bold(player_ptr, y, x, GB_FLOOR);
            } else {
                /* make granite outside so on_defeat_arena_monster works */
                place_bold(player_ptr, y, x, GB_EXTRA);
            }

            /* proper boundary for on_defeat_arena_monster */
            if (((y + rad) == center->y) || ((y - rad) == center->y) || ((x + rad) == center->x) || ((x - rad) == center->x)) {
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Find visible outer walls and set to be FEAT_OUTER */
    add_outer_wall(player_ptr, center->x, center->y, should_brighten, center->x - rad - 1, center->y - rad - 1, center->x + rad + 1, center->y + rad + 1);

    /* Check to see if there is room for an inner vault */
    auto is_empty = true;
    for (auto x = center->x - 2; x <= center->x + 2; x++) {
        if (!is_empty) {
            break;
        }

        for (auto y = center->y - 2; y <= center->y + 2; y++) {
            if (!floor.get_grid({ y, x }).is_floor()) {
                is_empty = false;
                break;
            }
        }
    }

    if (is_empty && one_in_(2)) {
        /* Build the vault */
        build_small_room(player_ptr, center->x, center->y);

        /* Place a treasure in the vault */
        place_object(player_ptr, center->y, center->x, 0);

        /* Let's guard the treasure well */
        vault_monsters(player_ptr, center->y, center->x, randint0(2) + 3);

        /* Traps naturally */
        vault_traps(floor, center->y, center->x, 4, 4, randint0(3) + 2);
    }

    return true;
}
