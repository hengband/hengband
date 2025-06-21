#include "room/rooms-special.h"
#include "floor//geometry.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind-hook.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

namespace {
void place_floor_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, grid, GB_FLOOR);
    grid.set_terrain_id(TerrainTag::GLASS_FLOOR);
}

void place_outer_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, grid, GB_OUTER);
    grid.set_terrain_id(TerrainTag::GLASS_WALL);
}

void place_inner_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, grid, GB_INNER);
    grid.set_terrain_id(TerrainTag::GLASS_WALL);
}

void place_inner_perm_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, grid, GB_INNER_PERM);
    grid.set_terrain_id(TerrainTag::PERMANENT_GLASS_WALL);
}
}

/*!
 * @brief タイプ15の部屋…ガラス部屋の生成 / Type 15 -- glass rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type15(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* Pick a room size */
    const auto width = rand_range(9, 13);
    const auto height = rand_range(9, 13);

    auto &floor = *player_ptr->current_floor_ptr;
    const auto center = find_space(player_ptr, dd_ptr, height + 2, width + 2);
    if (!center) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor.dun_level <= randint1(25)) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    const auto top = center->y - height / 2;
    const auto left = center->x - width / 2;
    const auto bottom = center->y + (height - 1) / 2;
    const auto right = center->x + (width - 1) / 2;

    /* Place a full floor under the room */
    for (auto y = top - 1; y <= bottom + 1; y++) {
        for (auto x = left - 1; x <= right + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_floor_glass(player_ptr, grid);
            grid.info |= (CAVE_ROOM);
            if (should_brighten) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (auto y = top - 1; y <= bottom + 1; y++) {
        place_outer_glass(player_ptr, floor.get_grid({ y, left - 1 }));
        place_outer_glass(player_ptr, floor.get_grid({ y, right + 1 }));
    }

    for (auto x = left - 1; x <= right + 1; x++) {
        place_outer_glass(player_ptr, floor.get_grid({ top - 1, x }));
        place_outer_glass(player_ptr, floor.get_grid({ bottom + 1, x }));
    }

    switch (randint1(3)) {
    case 1: /* 4 lite breathers + potion */
    {
        get_mon_num_prep_enum(player_ptr, MonraceHook::GLASS);

        /* Place fixed lite berathers */
        for (const auto &d_diag : Direction::directions_diag4()) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
            const auto pos = *center + d_diag.vec() * 2;
            if (MonraceList::is_valid(monrace_id)) {
                place_specific_monster(player_ptr, pos.y, pos.x, monrace_id, PM_ALLOW_SLEEP);
            }

            /* Walls around the breather */
            for (const auto &d : Direction::directions_8()) {
                place_inner_glass(player_ptr, floor.get_grid(pos + d.vec()));
            }
        }

        /* Walls around the potion */
        for (const auto &d : Direction::directions_4()) {
            place_inner_perm_glass(player_ptr, floor.get_grid(*center + d.vec() * 2));
            floor.get_grid(*center + d.vec()).info |= CAVE_ICKY;
        }

        /* Glass door */
        const auto &d = rand_choice(Direction::directions_4());
        const auto pos = *center + d.vec() * 2;
        place_secret_door(player_ptr, pos, DoorKind::GLASS_DOOR);
        if (floor.has_closed_door_at(pos)) {
            floor.get_grid(pos).set_terrain_id(TerrainTag::GLASS_WALL, TerrainKind::MIMIC);
        }

        /* Place a potion */
        place_object(player_ptr, *center, AM_NO_FIXED_ART, kind_is_potion);
        floor.get_grid(*center).info |= CAVE_ICKY;
    } break;

    case 2: /* 1 lite breather + random object */
    {
        /* Pillars */
        place_inner_glass(player_ptr, floor.get_grid({ top + 1, left + 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ top + 1, right - 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ bottom - 1, left + 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ bottom - 1, right - 1 }));
        get_mon_num_prep_enum(player_ptr, MonraceHook::GLASS);

        const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
        if (MonraceList::is_valid(monrace_id)) {
            place_specific_monster(player_ptr, center->y, center->x, monrace_id, 0L);
        }

        /* Walls around the breather */
        for (const auto &d : Direction::directions_8()) {
            place_inner_glass(player_ptr, floor.get_grid(*center + d.vec()));
        }

        /* Curtains around the breather */
        for (auto y = center->y - 1; y <= center->y + 1; y++) {
            place_secret_door(player_ptr, { y, center->x - 2 }, DoorKind::CURTAIN);
            place_secret_door(player_ptr, { y, center->x + 2 }, DoorKind::CURTAIN);
        }

        for (auto x = center->x - 1; x <= center->x + 1; x++) {
            place_secret_door(player_ptr, { center->y - 2, x }, DoorKind::CURTAIN);
            place_secret_door(player_ptr, { center->y + 2, x }, DoorKind::CURTAIN);
        }

        /* Place an object */
        place_object(player_ptr, *center, AM_NO_FIXED_ART);
        floor.get_grid(*center).info |= CAVE_ICKY;
    } break;

    case 3: /* 4 shards breathers + 2 potions */
    {
        /* Walls around the potion */
        for (auto y = center->y - 2; y <= center->y + 2; y++) {
            place_inner_glass(player_ptr, floor.get_grid({ y, center->x - 3 }));
            place_inner_glass(player_ptr, floor.get_grid({ y, center->x + 3 }));
        }

        for (auto x = center->x - 2; x <= center->x + 2; x++) {
            place_inner_glass(player_ptr, floor.get_grid({ center->y - 3, x }));
            place_inner_glass(player_ptr, floor.get_grid({ center->y + 3, x }));
        }

        for (const auto &d_diag : Direction::directions_diag4()) {
            place_inner_glass(player_ptr, floor.get_grid(*center + d_diag.vec() * 2));
        }

        get_mon_num_prep_enum(player_ptr, MonraceHook::SHARDS);

        /* Place shard berathers */
        for (const auto &d_diag : Direction::directions_diag4()) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
            const auto pos = *center + d_diag.vec();
            if (MonraceList::is_valid(monrace_id)) {
                place_specific_monster(player_ptr, pos.y, pos.x, monrace_id, 0L);
            }
        }

        /* Place two potions */
        if (one_in_(2)) {
            place_object(player_ptr, *center + Direction(4).vec(), AM_NO_FIXED_ART, kind_is_potion);
            place_object(player_ptr, *center + Direction(6).vec(), AM_NO_FIXED_ART, kind_is_potion);
        } else {
            place_object(player_ptr, *center + Direction(8).vec(), AM_NO_FIXED_ART, kind_is_potion);
            place_object(player_ptr, *center + Direction(2).vec(), AM_NO_FIXED_ART, kind_is_potion);
        }

        for (auto y = center->y - 2; y <= center->y + 2; y++) {
            for (auto x = center->x - 2; x <= center->x + 2; x++) {
                floor.get_grid({ y, x }).info |= CAVE_ICKY;
            }
        }
    } break;
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("ガラスの部屋が生成されました。", "Glass room was generated."));
    return true;
}
