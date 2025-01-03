#include "grid/door.h"
#include "dungeon/dungeon-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "room/door-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"

/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(PlayerType *player_ptr, POSITION x, POSITION y)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->grid_array[y][x].is_outer()) {
        return;
    }

    /* look at:
     *  x#x
     *  .#.
     *  x#x
     *
     *  where x=don't care
     *  .=floor, #=wall
     */

    if (floor_ptr->grid_array[y - 1][x].is_floor() && floor_ptr->grid_array[y + 1][x].is_floor() && floor_ptr->grid_array[y][x - 1].is_outer() && floor_ptr->grid_array[y][x + 1].is_outer()) {
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
    if (floor_ptr->grid_array[y - 1][x].is_outer() && floor_ptr->grid_array[y + 1][x].is_outer() && floor_ptr->grid_array[y][x - 1].is_floor() && floor_ptr->grid_array[y][x + 1].is_floor()) {
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
    auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D pos(y, x);
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    if (type == DOOR_DEFAULT) {
        type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                   ? DOOR_CURTAIN
                   : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
    }

    place_closed_door(player_ptr, pos.y, pos.x, type);
    auto &grid = floor.get_grid(pos);
    if (type != DOOR_CURTAIN) {
        grid.mimic = feat_wall_inner;
        if (grid.has_los_terrain(TerrainKind::MIMIC_RAW) && !grid.has_los_terrain()) {
            const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC_RAW);
            if (terrain_mimic.flags.has(TerrainCharacteristics::MOVE) || terrain_mimic.flags.has(TerrainCharacteristics::CAN_FLY)) {
                grid.feat = one_in_(2) ? grid.mimic : rand_choice(feat_ground_type);
            }

            grid.mimic = 0;
        }
    }

    grid.info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, pos.y, pos.x);
}

/*!
 * @brief 鍵のかかったドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 */
void place_locked_door(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    floor.set_terrain_id(pos, feat_locked_door_random(dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
    floor.get_grid(pos).info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, pos.y, pos.x);
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
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    grid.mimic = 0;
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    auto type = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                    ? DOOR_CURTAIN
                    : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);

    auto tmp = randint0(1000);
    const auto &terrains = TerrainList::get_instance();
    const auto terrain_none = terrains.get_terrain_id(TerrainTag::NONE);
    auto terrain_id = terrain_none;
    if (tmp < 300) {
        terrain_id = feat_door[type].open;
    } else if (tmp < 400) {
        terrain_id = feat_door[type].broken;
    } else if (tmp < 600) {
        place_closed_door(player_ptr, y, x, type);
        if (type != DOOR_CURTAIN) {
            grid.mimic = room ? feat_wall_outer : rand_choice(feat_wall_type);
            if (grid.has_los_terrain(TerrainKind::MIMIC_RAW) && !grid.has_los_terrain()) {
                const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC_RAW);
                if (terrain_mimic.flags.has(TerrainCharacteristics::MOVE) || terrain_mimic.flags.has(TerrainCharacteristics::CAN_FLY)) {
                    grid.feat = one_in_(2) ? grid.mimic : rand_choice(feat_ground_type);
                }
                grid.mimic = 0;
            }
        }
    } else {
        place_closed_door(player_ptr, y, x, type);
    }

    if (tmp >= 400) {
        delete_monster(player_ptr, y, x);
        return;
    }

    if (terrain_id == terrain_none) {
        place_bold(player_ptr, y, x, GB_FLOOR);
    } else {
        floor.set_terrain_id(pos, terrain_id);
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
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    const auto &terrains = TerrainList::get_instance();
    const auto terrain_none = terrains.get_terrain_id(TerrainTag::NONE);
    auto tmp = randint0(400);
    auto terrain_id = terrain_none;
    if (tmp < 300) {
        terrain_id = feat_door[type].closed;
    } else if (tmp < 399) {
        terrain_id = feat_locked_door_random(type);
    } else {
        terrain_id = feat_jammed_door_random(type);
    }

    if (terrain_id == terrain_none) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    cave_set_feat(player_ptr, pos.y, pos.x, terrain_id);
    floor.get_grid(pos).info &= ~(CAVE_MASK);
}
