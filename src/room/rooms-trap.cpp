#include "room/rooms-trap.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/floor-generator.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/space-finder.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief タイプ14の部屋…特殊トラップ部屋の生成 / Type 14 -- trapped rooms
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * A special trap is placed at center of the room
 */
bool build_type14(PlayerType *player_ptr, DungeonData *dd_ptr)
{
    /* Pick a room size */
    const auto room_seed_y1 = randint1(4);
    const auto room_seed_x1 = randint1(11);
    const auto room_seed_y2 = randint1(3);
    const auto room_seed_x2 = randint1(11);

    const auto xsize = room_seed_x1 + room_seed_x2 + 1;
    const auto ysize = room_seed_y1 + room_seed_y2 + 1;

    const auto center = find_space(player_ptr, dd_ptr, ysize + 2, xsize + 2);
    if (!center) {
        return false;
    }

    /* Choose lite or dark */
    auto &floor = *player_ptr->current_floor_ptr;
    const auto light = ((floor.dun_level <= randint1(25)) && floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS));

    /* Get corner values */
    const auto y1 = center->y - ysize / 2;
    const auto x1 = center->x - xsize / 2;
    const auto y2 = center->y + (ysize - 1) / 2;
    const auto x2 = center->x + (xsize - 1) / 2;

    /* Place a full floor under the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        for (auto x = x1 - 1; x <= x2 + 1; x++) {
            auto &grid = floor.get_grid({ y, x });
            place_grid(player_ptr, &grid, GB_FLOOR);
            grid.info |= (CAVE_ROOM);
            if (light) {
                grid.info |= (CAVE_GLOW);
            }
        }
    }

    /* Walls around the room */
    for (auto y = y1 - 1; y <= y2 + 1; y++) {
        place_grid(player_ptr, &floor.get_grid({ y, x1 - 1 }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y, x2 + 1 }), GB_OUTER);
    }

    for (auto x = x1 - 1; x <= x2 + 1; x++) {
        place_grid(player_ptr, &floor.get_grid({ y1 - 1, x }), GB_OUTER);
        place_grid(player_ptr, &floor.get_grid({ y2 + 1, x }), GB_OUTER);
    }

    const auto trap = floor.dun_level < 30 + randint1(30) ? feat_trap_piranha : feat_trap_armageddon;
    const auto trap_y = rand_spread(center->y, ysize / 4);
    const auto trap_x = rand_spread(center->x, xsize / 4);
    auto &grid = floor.get_grid({ trap_y, trap_x });
    grid.mimic = grid.feat;
    grid.feat = trap;
    constexpr auto fmt = _("%sの部屋が生成されました。", "Room of %s was generated.");
    msg_format_wizard(player_ptr, CHEAT_DUNGEON, fmt, TerrainList::get_instance().get_terrain(trap).name.data());
    return true;
}
