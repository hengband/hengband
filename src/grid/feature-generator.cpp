#include "grid/feature-generator.h"
#include "dungeon/dungeon-flag-mask.h"
#include "dungeon/quest.h"
#include "floor/dungeon-tunnel-util.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "grid/door.h"
#include "room/lake-types.h"
#include "room/rooms-builder.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

static bool decide_cavern(const FloorType &floor, const DungeonDefinition &dungeon, const DungeonData &dd)
{
    constexpr auto can_become_cavern = 20;
    auto should_build_cavern = floor.dun_level > can_become_cavern;
    should_build_cavern &= !dd.empty_level;
    should_build_cavern &= dungeon.flags.has(DungeonFeatureType::CAVERN);
    should_build_cavern &= dd.laketype == 0;
    should_build_cavern &= !dd.destroyed;
    should_build_cavern &= randint1(1000) < floor.dun_level;
    return should_build_cavern;
}

/*!
 * @brief フロアに破壊地形、洞窟、湖、溶岩、森林等を配置する.
 */
void gen_caverns_and_lakes(PlayerType *player_ptr, const DungeonDefinition &dungeon, DungeonData *dd_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    constexpr auto chance_destroyed = 18;
    if ((floor.dun_level > 30) && one_in_(chance_destroyed * 2) && small_levels && dungeon.flags.has(DungeonFeatureType::DESTROY)) {
        dd_ptr->destroyed = true;
        build_lake(player_ptr, one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
    }

    constexpr auto chance_water = 24;
    if (one_in_(chance_water) && !dd_ptr->empty_level && !dd_ptr->destroyed && dungeon.flags.has_any_of(DF_LAKE_MASK)) {
        auto count = dungeon.calc_cavern_terrains();
        if (dungeon.flags.has(DungeonFeatureType::LAKE_LAVA)) {
            if ((floor.dun_level > 80) && (randint0(count) < 2)) {
                dd_ptr->laketype = LAKE_T_LAVA;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor.dun_level > 80) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_FIRE_VAULT;
            }

            count--;
        }

        if (dungeon.flags.has(DungeonFeatureType::LAKE_WATER) && !dd_ptr->laketype) {
            if ((floor.dun_level > 50) && randint0(count) < 2) {
                dd_ptr->laketype = LAKE_T_WATER;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor.dun_level > 50) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_WATER_VAULT;
            }

            count--;
        }

        if (dungeon.flags.has(DungeonFeatureType::LAKE_RUBBLE) && !dd_ptr->laketype) {
            if ((floor.dun_level > 35) && (randint0(count) < 2)) {
                dd_ptr->laketype = LAKE_T_CAVE;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor.dun_level > 35) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_EARTH_VAULT;
            }

            count--;
        }

        if ((floor.dun_level > 5) && dungeon.flags.has(DungeonFeatureType::LAKE_TREE) && !dd_ptr->laketype) {
            dd_ptr->laketype = LAKE_T_AIR_VAULT;
        }

        if (dd_ptr->laketype) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("湖を生成します。", "Lake on the level."));
            build_lake(player_ptr, dd_ptr->laketype);
        }
    }

    const auto should_build_cavern = decide_cavern(floor, dungeon, *dd_ptr);
    if (should_build_cavern) {
        dd_ptr->cavern = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("洞窟を生成。", "Cavern on level."));
        build_cavern(player_ptr);
    }

    if (inside_quest(floor.get_quest_id())) {
        dd_ptr->destroyed = false;
    }
}

/*!
 * @brief 隣接4マスに存在する通路の数を返す / Count the number of "corridor" grids adjacent to the given grid.
 * @param y1 基準となるマスのY座標
 * @param x1 基準となるマスのX座標
 * @return 通路の数
 * @note Assumes "in_bounds(y1, x1)"
 * @details
 * XXX XXX This routine currently only counts actual "empty floor"\n
 * grids which are not in rooms.  We might want to also count stairs,\n
 * open doors, closed doors, etc.
 */
static int next_to_corr(const FloorType &floor, POSITION y1, POSITION x1)
{
    int k = 0;
    for (const auto &d : Direction::directions_4()) {
        const auto pos = Pos2D(y1, x1) + d.vec();
        const auto &grid = floor.get_grid(pos);
        if (grid.has(TerrainCharacteristics::WALL) || !grid.is_floor() || grid.is_room()) {
            continue;
        }

        k++;
    }

    return k;
}

/*!
 * @brief ドアを設置可能な地形かを返す / Determine if the given location is "between" two walls, and "next to" two corridor spaces.
 * @param y 判定を行いたいマスのY座標
 * @param x 判定を行いたいマスのX座標
 * @return ドアを設置可能ならばTRUEを返す
 * @details まず垂直方向に、次に水平方向に調べる
 */
static bool possible_doorway(const FloorType &floor, POSITION y, POSITION x)
{
    if (next_to_corr(floor, y, x) < 2) {
        return false;
    }

    constexpr auto wall = TerrainCharacteristics::WALL;
    if (floor.has_terrain_characteristics({ y - 1, x }, wall) && floor.has_terrain_characteristics({ y + 1, x }, wall)) {
        return true;
    }

    if (floor.has_terrain_characteristics({ y, x - 1 }, wall) && floor.has_terrain_characteristics({ y, x + 1 }, wall)) {
        return true;
    }

    return false;
}

/*!
 * @brief ドアの設置を試みる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 設置を行いたいマスの座標
 */
void try_door(PlayerType *player_ptr, dt_type *dt_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE) || floor.has_terrain_characteristics(pos, TerrainCharacteristics::WALL) || floor.get_grid(pos).is_room()) {
        return;
    }

    auto can_place_door = evaluate_percent(dt_ptr->dun_tun_jct);
    can_place_door &= possible_doorway(floor, pos.y, pos.x);
    can_place_door &= floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::NO_DOORS);
    if (can_place_door) {
        place_random_door(player_ptr, pos, false);
    }
}
