#include "grid/feature-generator.h"
#include "dungeon/dungeon-flag-mask.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "grid/door.h"
#include "grid/feature-flag-types.h"
#include "room/lake-types.h"
#include "room/rooms-builder.h"
#include "system/dungeon-data-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

/*
 * @brief 洞窟らしい地形 (湖、溶岩、瓦礫、森林)の個数を決める
 * @param d_ref ダンジョンへの参照
 * @return briefで定義した個数
 */
static int calc_cavern_terrains(const dungeon_type &d_ref)
{
    auto count = 0;
    if (d_ref.flags.has(DungeonFeatureType::LAKE_WATER)) {
        count += 3;
    }

    if (d_ref.flags.has(DungeonFeatureType::LAKE_LAVA)) {
        count += 3;
    }

    if (d_ref.flags.has(DungeonFeatureType::LAKE_RUBBLE)) {
        count += 3;
    }

    if (d_ref.flags.has(DungeonFeatureType::LAKE_TREE)) {
        count += 3;
    }

    return count;
}

static bool decide_cavern(const FloorType &floor_ref, const dungeon_type &dungeon_ref, const dun_data_type &dd_ref)
{
    constexpr auto can_become_cavern = 20;
    auto should_build_cavern = floor_ref.dun_level > can_become_cavern;
    should_build_cavern &= !dd_ref.empty_level;
    should_build_cavern &= dungeon_ref.flags.has(DungeonFeatureType::CAVERN);
    should_build_cavern &= dd_ref.laketype == 0;
    should_build_cavern &= !dd_ref.destroyed;
    should_build_cavern &= randint1(1000) < floor_ref.dun_level;
    return should_build_cavern;
}

/*!
 * @brief フロアに破壊地形、洞窟、湖、溶岩、森林等を配置する.
 */
void gen_caverns_and_lakes(PlayerType *player_ptr, dungeon_type *dungeon_ptr, dun_data_type *dd_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    constexpr auto chance_destroyed = 18;
    if ((floor_ptr->dun_level > 30) && one_in_(chance_destroyed * 2) && small_levels && dungeon_ptr->flags.has(DungeonFeatureType::DESTROY)) {
        dd_ptr->destroyed = true;
        build_lake(player_ptr, one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
    }

    constexpr auto chance_water = 24;
    if (one_in_(chance_water) && !dd_ptr->empty_level && !dd_ptr->destroyed && dungeon_ptr->flags.has_any_of(DF_LAKE_MASK)) {
        auto count = calc_cavern_terrains(*dungeon_ptr);
        if (dungeon_ptr->flags.has(DungeonFeatureType::LAKE_LAVA)) {
            if ((floor_ptr->dun_level > 80) && (randint0(count) < 2)) {
                dd_ptr->laketype = LAKE_T_LAVA;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 80) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_FIRE_VAULT;
            }

            count--;
        }

        if (dungeon_ptr->flags.has(DungeonFeatureType::LAKE_WATER) && !dd_ptr->laketype) {
            if ((floor_ptr->dun_level > 50) && randint0(count) < 2) {
                dd_ptr->laketype = LAKE_T_WATER;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 50) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_WATER_VAULT;
            }

            count--;
        }

        if (dungeon_ptr->flags.has(DungeonFeatureType::LAKE_RUBBLE) && !dd_ptr->laketype) {
            if ((floor_ptr->dun_level > 35) && (randint0(count) < 2)) {
                dd_ptr->laketype = LAKE_T_CAVE;
            }

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 35) && one_in_(count)) {
                dd_ptr->laketype = LAKE_T_EARTH_VAULT;
            }

            count--;
        }

        if ((floor_ptr->dun_level > 5) && dungeon_ptr->flags.has(DungeonFeatureType::LAKE_TREE) && !dd_ptr->laketype) {
            dd_ptr->laketype = LAKE_T_AIR_VAULT;
        }

        if (dd_ptr->laketype) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("湖を生成します。", "Lake on the level."));
            build_lake(player_ptr, dd_ptr->laketype);
        }
    }

    const auto should_build_cavern = decide_cavern(*floor_ptr, *dungeon_ptr, *dd_ptr);
    if (should_build_cavern) {
        dd_ptr->cavern = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("洞窟を生成。", "Cavern on level."));
        build_cavern(player_ptr);
    }

    if (inside_quest(quest_number(player_ptr, floor_ptr->dun_level))) {
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
static int next_to_corr(FloorType *floor_ptr, POSITION y1, POSITION x1)
{
    int k = 0;
    for (int i = 0; i < 4; i++) {
        POSITION y = y1 + ddy_ddd[i];
        POSITION x = x1 + ddx_ddd[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (g_ptr->cave_has_flag(TerrainCharacteristics::WALL) || !g_ptr->is_floor() || g_ptr->is_room()) {
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
static bool possible_doorway(FloorType *floor_ptr, POSITION y, POSITION x)
{
    if (next_to_corr(floor_ptr, y, x) < 2) {
        return false;
    }

    if (cave_has_flag_bold(floor_ptr, y - 1, x, TerrainCharacteristics::WALL) && cave_has_flag_bold(floor_ptr, y + 1, x, TerrainCharacteristics::WALL)) {
        return true;
    }

    if (cave_has_flag_bold(floor_ptr, y, x - 1, TerrainCharacteristics::WALL) && cave_has_flag_bold(floor_ptr, y, x + 1, TerrainCharacteristics::WALL)) {
        return true;
    }

    return false;
}

/*!
 * @brief ドアの設置を試みる / Places door at y, x position if at least 2 walls found
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 設置を行いたいマスのY座標
 * @param x 設置を行いたいマスのX座標
 */
void try_door(PlayerType *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x) || cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::WALL) || floor_ptr->grid_array[y][x].is_room()) {
        return;
    }

    bool can_place_door = randint0(100) < dt_ptr->dun_tun_jct;
    can_place_door &= possible_doorway(floor_ptr, y, x);
    can_place_door &= dungeons_info[player_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::NO_DOORS);
    if (can_place_door) {
        place_random_door(player_ptr, y, x, false);
    }
}
