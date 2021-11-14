#include "room/rooms-trap.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/floor-generator.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/space-finder.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief タイプ14の部屋…特殊トラップ部屋の生成 / Type 14 -- trapped rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * A special trap is placed at center of the room
 */
bool build_type14(PlayerType *player_ptr, dun_data_type *dd_ptr)
{
    POSITION y, x, y2, x2, yval, xval;
    POSITION y1, x1, xsize, ysize;

    bool light;

    grid_type *g_ptr;
    int16_t trap;

    /* Pick a room size */
    y1 = randint1(4);
    x1 = randint1(11);
    y2 = randint1(3);
    x2 = randint1(11);

    xsize = x1 + x2 + 1;
    ysize = y1 + y2 + 1;

    /* Find and reserve some space in the dungeon.  Get center of room. */
    if (!find_space(player_ptr, dd_ptr, &yval, &xval, ysize + 2, xsize + 2))
        return false;

    /* Choose lite or dark */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    light = ((floor_ptr->dun_level <= randint1(25)) && d_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS));

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
            if (light)
                g_ptr->info |= (CAVE_GLOW);
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

    if (floor_ptr->dun_level < 30 + randint1(30))
        trap = feat_trap_piranha;
    else
        trap = feat_trap_armageddon;

    /* Place a special trap */
    g_ptr = &floor_ptr->grid_array[rand_spread(yval, ysize / 4)][rand_spread(xval, xsize / 4)];
    g_ptr->mimic = g_ptr->feat;
    g_ptr->feat = trap;

    msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("%sの部屋が生成されました。", "Room of %s was generated."), f_info[trap].name.c_str());
    return true;
}
