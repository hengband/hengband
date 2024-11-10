#include "room/rooms-special.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor//geometry.h"
#include "floor/floor-generator.h"
#include "game-option/cheat-types.h"
#include "grid/door.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind-hook.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "wizard/wizard-messages.h"

namespace {
void place_floor_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, &grid, GB_FLOOR);
    grid.feat = feat_glass_floor;
}

void place_outer_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, &grid, GB_OUTER);
    grid.feat = feat_glass_wall;
}

void place_inner_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, &grid, GB_INNER);
    grid.feat = feat_glass_wall;
}

void place_inner_perm_glass(PlayerType *player_ptr, Grid &grid)
{
    place_grid(player_ptr, &grid, GB_INNER_PERM);
    grid.feat = feat_permanent_glass_wall;
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

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto &floor = *player_ptr->current_floor_ptr;
    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, height + 2, width + 2)) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor.dun_level <= randint1(25)) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    const auto top = yval - height / 2;
    const auto left = xval - width / 2;
    const auto bottom = yval + (height - 1) / 2;
    const auto right = xval + (width - 1) / 2;

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
        get_mon_num_prep(player_ptr, vault_aux_lite, nullptr);

        /* Place fixed lite berathers */
        for (auto dir1 = 4; dir1 < 8; dir1++) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
            const auto y = yval + 2 * ddy_ddd[dir1];
            const auto x = xval + 2 * ddx_ddd[dir1];
            if (MonraceList::is_valid(monrace_id)) {
                place_specific_monster(player_ptr, y, x, monrace_id, PM_ALLOW_SLEEP);
            }

            /* Walls around the breather */
            for (auto dir2 = 0; dir2 < 8; dir2++) {
                place_inner_glass(player_ptr, floor.get_grid({ y + ddy_ddd[dir2], x + ddx_ddd[dir2] }));
            }
        }

        /* Walls around the potion */
        for (auto dir1 = 0; dir1 < 4; dir1++) {
            place_inner_perm_glass(player_ptr, floor.get_grid({ yval + 2 * ddy_ddd[dir1], xval + 2 * ddx_ddd[dir1] }));
            floor.get_grid({ yval + ddy_ddd[dir1], xval + ddx_ddd[dir1] }).info |= CAVE_ICKY;
        }

        /* Glass door */
        const auto dir1 = randint0(4);
        const auto y = yval + 2 * ddy_ddd[dir1];
        const auto x = xval + 2 * ddx_ddd[dir1];
        place_secret_door(player_ptr, y, x, DOOR_GLASS_DOOR);
        auto &grid = floor.get_grid({ y, x });
        if (is_closed_door(player_ptr, grid.feat)) {
            grid.mimic = feat_glass_wall;
        }

        /* Place a potion */
        get_obj_index_hook = kind_is_potion;
        place_object(player_ptr, yval, xval, AM_NO_FIXED_ART);
        floor.get_grid({ yval, xval }).info |= CAVE_ICKY;
    } break;

    case 2: /* 1 lite breather + random object */
    {
        /* Pillars */
        place_inner_glass(player_ptr, floor.get_grid({ top + 1, left + 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ top + 1, right - 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ bottom - 1, left + 1 }));
        place_inner_glass(player_ptr, floor.get_grid({ bottom - 1, right - 1 }));
        get_mon_num_prep(player_ptr, vault_aux_lite, nullptr);

        const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
        if (MonraceList::is_valid(monrace_id)) {
            place_specific_monster(player_ptr, yval, xval, monrace_id, 0L);
        }

        /* Walls around the breather */
        for (auto dir1 = 0; dir1 < 8; dir1++) {
            place_inner_glass(player_ptr, floor.get_grid({ yval + ddy_ddd[dir1], xval + ddx_ddd[dir1] }));
        }

        /* Curtains around the breather */
        for (auto y = yval - 1; y <= yval + 1; y++) {
            place_secret_door(player_ptr, y, xval - 2, DOOR_CURTAIN);
            place_secret_door(player_ptr, y, xval + 2, DOOR_CURTAIN);
        }

        for (auto x = xval - 1; x <= xval + 1; x++) {
            place_secret_door(player_ptr, yval - 2, x, DOOR_CURTAIN);
            place_secret_door(player_ptr, yval + 2, x, DOOR_CURTAIN);
        }

        /* Place an object */
        place_object(player_ptr, yval, xval, AM_NO_FIXED_ART);
        floor.get_grid({ yval, xval }).info |= CAVE_ICKY;
    } break;

    case 3: /* 4 shards breathers + 2 potions */
    {
        /* Walls around the potion */
        for (auto y = yval - 2; y <= yval + 2; y++) {
            place_inner_glass(player_ptr, floor.get_grid({ y, xval - 3 }));
            place_inner_glass(player_ptr, floor.get_grid({ y, xval + 3 }));
        }

        for (auto x = xval - 2; x <= xval + 2; x++) {
            place_inner_glass(player_ptr, floor.get_grid({ yval - 3, x }));
            place_inner_glass(player_ptr, floor.get_grid({ yval + 3, x }));
        }

        for (auto dir1 = 4; dir1 < 8; dir1++) {
            place_inner_glass(player_ptr, floor.get_grid({ yval + 2 * ddy_ddd[dir1], xval + 2 * ddx_ddd[dir1] }));
        }

        get_mon_num_prep(player_ptr, vault_aux_shards, nullptr);

        /* Place shard berathers */
        for (auto dir1 = 4; dir1 < 8; dir1++) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor.dun_level, 0);
            const auto y = yval + ddy_ddd[dir1];
            const auto x = xval + ddx_ddd[dir1];
            if (MonraceList::is_valid(monrace_id)) {
                place_specific_monster(player_ptr, y, x, monrace_id, 0L);
            }
        }

        /* Place two potions */
        if (one_in_(2)) {
            get_obj_index_hook = kind_is_potion;
            place_object(player_ptr, yval, xval - 1, AM_NO_FIXED_ART);
            get_obj_index_hook = kind_is_potion;
            place_object(player_ptr, yval, xval + 1, AM_NO_FIXED_ART);
        } else {
            get_obj_index_hook = kind_is_potion;
            place_object(player_ptr, yval - 1, xval, AM_NO_FIXED_ART);
            get_obj_index_hook = kind_is_potion;
            place_object(player_ptr, yval + 1, xval, AM_NO_FIXED_ART);
        }

        for (auto y = yval - 2; y <= yval + 2; y++) {
            for (auto x = xval - 2; x <= xval + 2; x++) {
                floor.get_grid({ y, x }).info |= CAVE_ICKY;
            }
        }
    } break;
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("ガラスの部屋が生成されました。", "Glass room was generated."));
    return true;
}
