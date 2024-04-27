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
#include "monster-race/monster-race.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object-enchant/item-apply-magic.h"
#include "object/object-kind-hook.h"
#include "room/door-definition.h"
#include "room/space-finder.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief タイプ15の部屋…ガラス部屋の生成 / Type 15 -- glass rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 */
bool build_type15(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    /* Pick a room size */
    const auto xsize = rand_range(9, 13);
    const auto ysize = rand_range(9, 13);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    int yval;
    int xval;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2)) {
        return false;
    }

    /* Choose lite or dark */
    const auto should_brighten = ((floor_ptr->dun_level <= randint1(25)) && floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    const auto y1 = yval - ysize / 2;
    const auto x1 = xval - xsize / 2;
    const auto y2 = yval + (ysize - 1) / 2;
    const auto x2 = xval + (xsize - 1) / 2;

    /* Place a full floor under the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_FLOOR);
            g_ptr->feat = feat_glass_floor;
            g_ptr->info |= (CAVE_ROOM);
            if (should_brighten) {
                g_ptr->info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        auto *g_ptr = &floor_ptr->grid_array[y][x1 - 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
    }

    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        auto *g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
    }

    switch (randint1(3)) {
    case 1: /* 4 lite breathers + potion */
    {
        get_mon_num_prep(player_ptr, vault_aux_lite, nullptr);

        /* Place fixed lite berathers */
        for (auto dir1 = 4; dir1 < 8; dir1++) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);
            auto y = yval + 2 * ddy_ddd[dir1];
            auto x = xval + 2 * ddx_ddd[dir1];
            if (MonsterRace(monrace_id).is_valid()) {
                place_specific_monster(player_ptr, 0, y, x, monrace_id, PM_ALLOW_SLEEP);
            }

            /* Walls around the breather */
            for (auto dir2 = 0; dir2 < 8; dir2++) {
                auto *g_ptr = &floor_ptr->grid_array[y + ddy_ddd[dir2]][x + ddx_ddd[dir2]];
                place_grid(player_ptr, g_ptr, GB_INNER);
                g_ptr->feat = feat_glass_wall;
            }
        }

        /* Walls around the potion */
        for (auto dir1 = 0; dir1 < 4; dir1++) {
            auto y = yval + 2 * ddy_ddd[dir1];
            auto x = xval + 2 * ddx_ddd[dir1];
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_INNER_PERM);
            g_ptr->feat = feat_permanent_glass_wall;
            floor_ptr->grid_array[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]].info |= (CAVE_ICKY);
        }

        /* Glass door */
        auto dir1 = randint0(4);
        auto y = yval + 2 * ddy_ddd[dir1];
        auto x = xval + 2 * ddx_ddd[dir1];
        place_secret_door(player_ptr, y, x, DOOR_GLASS_DOOR);
        auto *g_ptr = &floor_ptr->grid_array[y][x];
        if (is_closed_door(player_ptr, g_ptr->feat)) {
            g_ptr->mimic = feat_glass_wall;
        }

        /* Place a potion */
        get_obj_index_hook = kind_is_potion;
        place_object(player_ptr, yval, xval, AM_NO_FIXED_ART);
        floor_ptr->grid_array[yval][xval].info |= (CAVE_ICKY);
    } break;

    case 2: /* 1 lite breather + random object */
    {
        /* Pillars */
        auto *g_ptr = &floor_ptr->grid_array[y1 + 1][x1 + 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr->feat = feat_glass_wall;

        g_ptr = &floor_ptr->grid_array[y1 + 1][x2 - 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr->feat = feat_glass_wall;

        g_ptr = &floor_ptr->grid_array[y2 - 1][x1 + 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr->feat = feat_glass_wall;

        g_ptr = &floor_ptr->grid_array[y2 - 1][x2 - 1];
        place_grid(player_ptr, g_ptr, GB_INNER);
        g_ptr->feat = feat_glass_wall;
        get_mon_num_prep(player_ptr, vault_aux_lite, nullptr);

        const auto monrace_id = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);
        if (MonsterRace(monrace_id).is_valid()) {
            place_specific_monster(player_ptr, 0, yval, xval, monrace_id, 0L);
        }

        /* Walls around the breather */
        for (auto dir1 = 0; dir1 < 8; dir1++) {
            g_ptr = &floor_ptr->grid_array[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
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
        floor_ptr->grid_array[yval][xval].info |= (CAVE_ICKY);
    } break;

    case 3: /* 4 shards breathers + 2 potions */
    {
        /* Walls around the potion */
        for (auto y = yval - 2; y <= yval + 2; y++) {
            auto *g_ptr = &floor_ptr->grid_array[y][xval - 3];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
            g_ptr = &floor_ptr->grid_array[y][xval + 3];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        for (auto x = xval - 2; x <= xval + 2; x++) {
            auto *g_ptr = &floor_ptr->grid_array[yval - 3][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
            g_ptr = &floor_ptr->grid_array[yval + 3][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        for (auto dir1 = 4; dir1 < 8; dir1++) {
            auto *g_ptr = &floor_ptr->grid_array[yval + 2 * ddy_ddd[dir1]][xval + 2 * ddx_ddd[dir1]];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        get_mon_num_prep(player_ptr, vault_aux_shards, nullptr);

        /* Place shard berathers */
        for (auto dir1 = 4; dir1 < 8; dir1++) {
            const auto monrace_id = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);
            auto y = yval + ddy_ddd[dir1];
            auto x = xval + ddx_ddd[dir1];
            if (MonsterRace(monrace_id).is_valid()) {
                place_specific_monster(player_ptr, 0, y, x, monrace_id, 0L);
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
                floor_ptr->grid_array[y][x].info |= (CAVE_ICKY);
            }
        }
    } break;
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("ガラスの部屋が生成されました。", "Glass room was generated."));
    return true;
}
