#include "grid/door.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/door-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(PlayerType *player_ptr, POSITION x, POSITION y)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->grid_array[y][x].is_outer())
        return;

    /* look at:
     *  x#x
     *  .#.
     *  x#x
     *
     *  where x=don't care
     *  .=floor, #=wall
     */

    if (floor_ptr->grid_array[y - 1][x].is_floor() && floor_ptr->grid_array[y + 1][x].is_floor() && floor_ptr->grid_array[y][x - 1].is_outer()
        && floor_ptr->grid_array[y][x + 1].is_outer()) {
        place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
        place_bold(player_ptr, y, x - 1, GB_SOLID);
        place_bold(player_ptr, y, x + 1, GB_SOLID);
    }

    /* look at:
     *  x#x
     *  .#.
     *  x#x
     *
     *  where x = don't care
     *  .=floor, #=wall
     */
    if (floor_ptr->grid_array[y - 1][x].is_outer() && floor_ptr->grid_array[y + 1][x].is_outer() && floor_ptr->grid_array[y][x - 1].is_floor()
        && floor_ptr->grid_array[y][x + 1].is_floor()) {
        place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
        place_bold(player_ptr, y - 1, x, GB_SOLID);
        place_bold(player_ptr, y + 1, x, GB_SOLID);
    }
}

/*!
 * @brief 隠しドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param type DOOR_DEFAULT / DOOR_DOOR / DOOR_GLASS_DOOR / DOOR_CURTAIN のいずれか
 */
void place_secret_door(PlayerType *player_ptr, POSITION y, POSITION x, int type)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    if (type == DOOR_DEFAULT) {
        type = (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN) && one_in_(d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
            ? DOOR_CURTAIN
            : (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
    }

    place_closed_door(player_ptr, y, x, type);
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (type != DOOR_CURTAIN) {
        g_ptr->mimic = feat_wall_inner;
        if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat)) {
            if (f_info[g_ptr->mimic].flags.has(FloorFeatureType::MOVE) || f_info[g_ptr->mimic].flags.has(FloorFeatureType::CAN_FLY)) {
                g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
            }

            g_ptr->mimic = 0;
        }
    }

    g_ptr->info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, y, x);
}

/*!
 * @brief 鍵のかかったドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 */
void place_locked_door(PlayerType *player_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    set_cave_feat(floor_ptr, y, x, feat_locked_door_random(d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
    floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, y, x);
}

/*!
 * @brief 所定の位置にさまざまな状態や種類のドアを配置する / Place a random type of door at the given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param room 部屋に接している場合向けのドア生成か否か
 */
void place_random_door(PlayerType *player_ptr, POSITION y, POSITION x, bool room)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->mimic = 0;

    if (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    int type = (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::CURTAIN) && one_in_(d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
        ? DOOR_CURTAIN
        : (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

    int tmp = randint0(1000);
    FEAT_IDX feat = feat_none;
    if (tmp < 300) {
        feat = feat_door[type].open;
    } else if (tmp < 400) {
        feat = feat_door[type].broken;
    } else if (tmp < 600) {
        place_closed_door(player_ptr, y, x, type);

        if (type != DOOR_CURTAIN) {
            g_ptr->mimic = room ? feat_wall_outer : feat_wall_type[randint0(100)];
            if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat)) {
                if (f_info[g_ptr->mimic].flags.has(FloorFeatureType::MOVE) || f_info[g_ptr->mimic].flags.has(FloorFeatureType::CAN_FLY)) {
                    g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
                }
                g_ptr->mimic = 0;
            }
        }
    } else {
        place_closed_door(player_ptr, y, x, type);
    }

    if (tmp >= 400) {
        delete_monster(player_ptr, y, x);
        return;
    }

    if (feat == feat_none) {
        place_bold(player_ptr, y, x, GB_FLOOR);
    } else {
        set_cave_feat(floor_ptr, y, x, feat);
    }

    delete_monster(player_ptr, y, x);
}

/*!
 * @brief 所定の位置に各種の閉じたドアを配置する / Place a random type of normal door at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y ドアの配置を試みたいマスのY座標
 * @param x ドアの配置を試みたいマスのX座標
 * @param type ドアの地形ID
 */
void place_closed_door(PlayerType *player_ptr, POSITION y, POSITION x, int type)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_info[floor_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    int tmp = randint0(400);
    FEAT_IDX feat = feat_none;
    if (tmp < 300) {
        feat = feat_door[type].closed;
    } else if (tmp < 399) {
        feat = feat_locked_door_random(type);
    } else {
        feat = feat_jammed_door_random(type);
    }

    if (feat == feat_none) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    cave_set_feat(player_ptr, y, x, feat);
    floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
}
