#include "grid/feature-generator.h"
#include "dungeon/dungeon-flag-mask.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief フロアに洞窟や湖を配置する / Generate various caverns and lakes
 * @details There were moved from cave_gen().
 */
void gen_caverns_and_lakes(player_type *owner_ptr, dungeon_type *dungeon_ptr, dun_data_type *dd_ptr)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    if ((floor_ptr->dun_level > 30) && one_in_(DUN_DEST * 2) && small_levels && dungeon_ptr->flags.has(DF::DESTROY)) {
        dd_ptr->destroyed = true;
        build_lake(owner_ptr, one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
    }

    if (one_in_(LAKE_LEVEL) && !dd_ptr->empty_level && !dd_ptr->destroyed && dungeon_ptr->flags.has_any_of(DF_LAKE_MASK)) {
        int count = 0;
        if (dungeon_ptr->flags.has(DF::LAKE_WATER))
            count += 3;

        if (dungeon_ptr->flags.has(DF::LAKE_LAVA))
            count += 3;

        if (dungeon_ptr->flags.has(DF::LAKE_RUBBLE))
            count += 3;

        if (dungeon_ptr->flags.has(DF::LAKE_TREE))
            count += 3;

        if (dungeon_ptr->flags.has(DF::LAKE_LAVA)) {
            if ((floor_ptr->dun_level > 80) && (randint0(count) < 2))
                dd_ptr->laketype = LAKE_T_LAVA;

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 80) && one_in_(count))
                dd_ptr->laketype = LAKE_T_FIRE_VAULT;

            count--;
        }

        if (dungeon_ptr->flags.has(DF::LAKE_WATER) && !dd_ptr->laketype) {
            if ((floor_ptr->dun_level > 50) && randint0(count) < 2)
                dd_ptr->laketype = LAKE_T_WATER;

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 50) && one_in_(count))
                dd_ptr->laketype = LAKE_T_WATER_VAULT;

            count--;
        }

        if (dungeon_ptr->flags.has(DF::LAKE_RUBBLE) && !dd_ptr->laketype) {
            if ((floor_ptr->dun_level > 35) && (randint0(count) < 2))
                dd_ptr->laketype = LAKE_T_CAVE;

            count -= 2;
            if (!dd_ptr->laketype && (floor_ptr->dun_level > 35) && one_in_(count))
                dd_ptr->laketype = LAKE_T_EARTH_VAULT;

            count--;
        }

        if ((floor_ptr->dun_level > 5) && dungeon_ptr->flags.has(DF::LAKE_TREE) && !dd_ptr->laketype)
            dd_ptr->laketype = LAKE_T_AIR_VAULT;

        if (dd_ptr->laketype) {
            msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("湖を生成します。", "Lake on the level."));
            build_lake(owner_ptr, dd_ptr->laketype);
        }
    }

    if ((floor_ptr->dun_level > DUN_CAVERN) && !dd_ptr->empty_level && dungeon_ptr->flags.has(DF::CAVERN) && !dd_ptr->laketype && !dd_ptr->destroyed
        && (randint1(1000) < floor_ptr->dun_level)) {
        dd_ptr->cavern = true;
        msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("洞窟を生成。", "Cavern on level."));
        build_cavern(owner_ptr);
    }

    if (quest_number(owner_ptr, floor_ptr->dun_level))
        dd_ptr->destroyed = false;
}

bool has_river_flag(dungeon_type *dungeon_ptr)
{
    return dungeon_ptr->flags.has_any_of(DF_RIVER_MASK);
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
static int next_to_corr(floor_type *floor_ptr, POSITION y1, POSITION x1)
{
    int k = 0;
    for (int i = 0; i < 4; i++) {
        POSITION y = y1 + ddy_ddd[i];
        POSITION x = x1 + ddx_ddd[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (g_ptr->cave_has_flag(FF::WALL) || !g_ptr->is_floor() || g_ptr->is_room())
            continue;

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
static bool possible_doorway(floor_type *floor_ptr, POSITION y, POSITION x)
{
    if (next_to_corr(floor_ptr, y, x) < 2)
        return false;

    if (cave_has_flag_bold(floor_ptr, y - 1, x, FF::WALL) && cave_has_flag_bold(floor_ptr, y + 1, x, FF::WALL))
        return true;

    if (cave_has_flag_bold(floor_ptr, y, x - 1, FF::WALL) && cave_has_flag_bold(floor_ptr, y, x + 1, FF::WALL))
        return true;

    return false;
}

/*!
 * @brief ドアの設置を試みる / Places door at y, x position if at least 2 walls found
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 設置を行いたいマスのY座標
 * @param x 設置を行いたいマスのX座標
 */
void try_door(player_type *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x) || cave_has_flag_bold(floor_ptr, y, x, FF::WALL) || floor_ptr->grid_array[y][x].is_room())
        return;

    bool can_place_door = randint0(100) < dt_ptr->dun_tun_jct;
    can_place_door &= possible_doorway(floor_ptr, y, x);
    can_place_door &= d_info[player_ptr->dungeon_idx].flags.has_not(DF::NO_DOORS);
    if (can_place_door)
        place_random_door(player_ptr, y, x, false);
}
