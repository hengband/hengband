#include "room/rooms-special.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
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
    POSITION y, x, y2, x2, yval, xval;
    POSITION y1, x1, xsize, ysize;
    bool light;

    grid_type *g_ptr;

    /* Pick a room size */
    xsize = rand_range(9, 13);
    ysize = rand_range(9, 13);

    /* Find and reserve some space in the dungeon.  Get center of room. */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2)) {
        return false;
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
            g_ptr->feat = feat_glass_floor;
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
        g_ptr->feat = feat_glass_wall;
        g_ptr = &floor_ptr->grid_array[y][x2 + 1];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
    }

    for (x = x1 - 1; x <= x2 + 1; x++) {
        g_ptr = &floor_ptr->grid_array[y1 - 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
        g_ptr = &floor_ptr->grid_array[y2 + 1][x];
        place_grid(player_ptr, g_ptr, GB_OUTER);
        g_ptr->feat = feat_glass_wall;
    }

    switch (randint1(3)) {
    case 1: /* 4 lite breathers + potion */
    {
        DIRECTION dir1, dir2;
        get_mon_num_prep(player_ptr, vault_aux_lite, nullptr);

        /* Place fixed lite berathers */
        for (dir1 = 4; dir1 < 8; dir1++) {
            MonsterRaceId r_idx = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);

            y = yval + 2 * ddy_ddd[dir1];
            x = xval + 2 * ddx_ddd[dir1];
            if (MonsterRace(r_idx).is_valid()) {
                place_monster_aux(player_ptr, 0, y, x, r_idx, PM_ALLOW_SLEEP);
            }

            /* Walls around the breather */
            for (dir2 = 0; dir2 < 8; dir2++) {
                g_ptr = &floor_ptr->grid_array[y + ddy_ddd[dir2]][x + ddx_ddd[dir2]];
                place_grid(player_ptr, g_ptr, GB_INNER);
                g_ptr->feat = feat_glass_wall;
            }
        }

        /* Walls around the potion */
        for (dir1 = 0; dir1 < 4; dir1++) {
            y = yval + 2 * ddy_ddd[dir1];
            x = xval + 2 * ddx_ddd[dir1];
            g_ptr = &floor_ptr->grid_array[y][x];
            place_grid(player_ptr, g_ptr, GB_INNER_PERM);
            g_ptr->feat = feat_permanent_glass_wall;
            floor_ptr->grid_array[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]].info |= (CAVE_ICKY);
        }

        /* Glass door */
        dir1 = randint0(4);
        y = yval + 2 * ddy_ddd[dir1];
        x = xval + 2 * ddx_ddd[dir1];
        place_secret_door(player_ptr, y, x, DOOR_GLASS_DOOR);
        g_ptr = &floor_ptr->grid_array[y][x];
        if (is_closed_door(player_ptr, g_ptr->feat)) {
            g_ptr->mimic = feat_glass_wall;
        }

        /* Place a potion */
        get_obj_num_hook = kind_is_potion;
        place_object(player_ptr, yval, xval, AM_NO_FIXED_ART);
        floor_ptr->grid_array[yval][xval].info |= (CAVE_ICKY);
    } break;

    case 2: /* 1 lite breather + random object */
    {
        MonsterRaceId r_idx;
        DIRECTION dir1;

        /* Pillars */
        g_ptr = &floor_ptr->grid_array[y1 + 1][x1 + 1];
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

        r_idx = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);
        if (MonsterRace(r_idx).is_valid()) {
            place_monster_aux(player_ptr, 0, yval, xval, r_idx, 0L);
        }

        /* Walls around the breather */
        for (dir1 = 0; dir1 < 8; dir1++) {
            g_ptr = &floor_ptr->grid_array[yval + ddy_ddd[dir1]][xval + ddx_ddd[dir1]];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        /* Curtains around the breather */
        for (y = yval - 1; y <= yval + 1; y++) {
            place_secret_door(player_ptr, y, xval - 2, DOOR_CURTAIN);
            place_secret_door(player_ptr, y, xval + 2, DOOR_CURTAIN);
        }

        for (x = xval - 1; x <= xval + 1; x++) {
            place_secret_door(player_ptr, yval - 2, x, DOOR_CURTAIN);
            place_secret_door(player_ptr, yval + 2, x, DOOR_CURTAIN);
        }

        /* Place an object */
        place_object(player_ptr, yval, xval, AM_NO_FIXED_ART);
        floor_ptr->grid_array[yval][xval].info |= (CAVE_ICKY);
    } break;

    case 3: /* 4 shards breathers + 2 potions */
    {
        DIRECTION dir1;

        /* Walls around the potion */
        for (y = yval - 2; y <= yval + 2; y++) {
            g_ptr = &floor_ptr->grid_array[y][xval - 3];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
            g_ptr = &floor_ptr->grid_array[y][xval + 3];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        for (x = xval - 2; x <= xval + 2; x++) {
            g_ptr = &floor_ptr->grid_array[yval - 3][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
            g_ptr = &floor_ptr->grid_array[yval + 3][x];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        for (dir1 = 4; dir1 < 8; dir1++) {
            g_ptr = &floor_ptr->grid_array[yval + 2 * ddy_ddd[dir1]][xval + 2 * ddx_ddd[dir1]];
            place_grid(player_ptr, g_ptr, GB_INNER);
            g_ptr->feat = feat_glass_wall;
        }

        get_mon_num_prep(player_ptr, vault_aux_shards, nullptr);

        /* Place shard berathers */
        for (dir1 = 4; dir1 < 8; dir1++) {
            MonsterRaceId r_idx = get_mon_num(player_ptr, 0, floor_ptr->dun_level, 0);

            y = yval + ddy_ddd[dir1];
            x = xval + ddx_ddd[dir1];
            if (MonsterRace(r_idx).is_valid()) {
                place_monster_aux(player_ptr, 0, y, x, r_idx, 0L);
            }
        }

        /* Place two potions */
        if (one_in_(2)) {
            get_obj_num_hook = kind_is_potion;
            place_object(player_ptr, yval, xval - 1, AM_NO_FIXED_ART);
            get_obj_num_hook = kind_is_potion;
            place_object(player_ptr, yval, xval + 1, AM_NO_FIXED_ART);
        } else {
            get_obj_num_hook = kind_is_potion;
            place_object(player_ptr, yval - 1, xval, AM_NO_FIXED_ART);
            get_obj_num_hook = kind_is_potion;
            place_object(player_ptr, yval + 1, xval, AM_NO_FIXED_ART);
        }

        for (y = yval - 2; y <= yval + 2; y++) {
            for (x = xval - 2; x <= xval + 2; x++) {
                floor_ptr->grid_array[y][x].info |= (CAVE_ICKY);
            }
        }
    } break;
    }

    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("ガラスの部屋が生成されました。", "Glass room was generated."));
    return true;
}
