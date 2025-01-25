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

/*!
 * @brief ドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置先座標
 * @details ドアの配置は、座標が外壁であることを前提としている
 *
 * look at:
 *   x#x
 *   .#.
 *   x#x
 *
 *   where 'x'=don't care, '.'=floor, '#'=wall
 */
void add_door(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.get_grid(pos).is_outer()) {
        return;
    }

    const auto pos_top = pos + Pos2DVec(-1, 0);
    const auto pos_bottom = pos + Pos2DVec(1, 0);
    const auto pos_left = pos + Pos2DVec(0, -1);
    const auto pos_right = pos + Pos2DVec(0, 1);
    if (floor.get_grid(pos_top).is_floor() && floor.get_grid(pos_bottom).is_floor() && floor.get_grid(pos_left).is_outer() && floor.get_grid(pos_right).is_outer()) {
        place_secret_door(player_ptr, pos);
        place_bold(player_ptr, pos_left.y, pos_left.x, GB_SOLID);
        place_bold(player_ptr, pos_right.y, pos_right.x, GB_SOLID);
    }

    if (floor.get_grid(pos_top).is_outer() && floor.get_grid(pos_bottom).is_outer() && floor.get_grid(pos_left).is_floor() && floor.get_grid(pos_right).is_floor()) {
        place_secret_door(player_ptr, pos);
        place_bold(player_ptr, pos_top.y, pos_top.x, GB_SOLID);
        place_bold(player_ptr, pos_bottom.y, pos_bottom.x, GB_SOLID);
    }
}

/*!
 * @brief 隠しドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置先座標
 * @param door_kind ドア種別
 */
void place_secret_door(PlayerType *player_ptr, const Pos2D &pos, std::optional<DoorKind> door_kind_initial)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    const auto door_kind = door_kind_initial ? *door_kind_initial : dungeon.select_door_kind();
    place_closed_door(player_ptr, pos, door_kind);
    auto &grid = floor.get_grid(pos);
    if (door_kind != DoorKind::CURTAIN) {
        grid.set_terrain_id(dungeon.inner_wall, TerrainKind::MIMIC);
        if (grid.has_los_terrain(TerrainKind::MIMIC_RAW) && !grid.has_los_terrain()) {
            const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC_RAW);
            if (terrain_mimic.flags.has(TerrainCharacteristics::MOVE) || terrain_mimic.flags.has(TerrainCharacteristics::CAN_FLY)) {
                const auto terrain_id = one_in_(2) ? grid.mimic : dungeon.select_floor_terrain_id();
                grid.set_terrain_id(terrain_id);
            }

            grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
        }
    }

    grid.info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, pos.y, pos.x);
}

/*!
 * @brief 鍵のかかったドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置先座標
 */
void place_locked_door(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    const auto door_kind = dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DoorKind::GLASS_DOOR : DoorKind::DOOR;
    floor.set_terrain_id_at(pos, Doors::get_instance().select_locked_tag(door_kind));
    floor.get_grid(pos).info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, pos.y, pos.x);
}

/*!
 * @brief 所定の位置にさまざまな状態や種類のドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置先座標
 * @param room 部屋に接している場合向けのドア生成か否か
 */
void place_random_door(PlayerType *player_ptr, const Pos2D &pos, bool is_room_door)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    const auto door_kind = (dungeon.flags.has(DungeonFeatureType::CURTAIN) && one_in_(dungeon.flags.has(DungeonFeatureType::NO_CAVE) ? 16 : 256))
                               ? DoorKind::CURTAIN
                               : (dungeon.flags.has(DungeonFeatureType::GLASS_DOOR) ? DoorKind::GLASS_DOOR : DoorKind::DOOR);
    const auto tmp = randint0(1000);
    auto tag = TerrainTag::NONE;
    const auto &door = Doors::get_instance().get_door(door_kind);
    if (tmp < 300) {
        tag = door.open;
    } else if (tmp < 400) {
        tag = door.broken;
    } else if (tmp < 600) {
        place_closed_door(player_ptr, pos, door_kind);
        if (door_kind != DoorKind::CURTAIN) {
            const auto mimic_terrain_id = is_room_door ? dungeon.outer_wall : dungeon.select_wall_terrain_id();
            grid.set_terrain_id(mimic_terrain_id, TerrainKind::MIMIC);
            if (grid.has_los_terrain(TerrainKind::MIMIC_RAW) && !grid.has_los_terrain()) {
                const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC_RAW);
                if (terrain_mimic.flags.has(TerrainCharacteristics::MOVE) || terrain_mimic.flags.has(TerrainCharacteristics::CAN_FLY)) {
                    const auto terrain_id = one_in_(2) ? grid.mimic : dungeon.select_floor_terrain_id();
                    grid.set_terrain_id(terrain_id);
                }

                grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
            }
        }
    } else {
        place_closed_door(player_ptr, pos, door_kind);
    }

    if (tmp >= 400) {
        delete_monster(player_ptr, pos.y, pos.x);
        return;
    }

    if (tag == TerrainTag::NONE) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
    } else {
        floor.set_terrain_id_at(pos, tag);
    }

    delete_monster(player_ptr, pos.y, pos.x);
}

/*!
 * @brief 所定の位置に各種の閉じたドアを配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 配置先座標
 * @param door_kind ドア種別
 */
void place_closed_door(PlayerType *player_ptr, const Pos2D &pos, DoorKind door_kind)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_DOORS)) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    const auto &doors = Doors::get_instance();
    const auto tmp = randint0(400);
    auto tag = TerrainTag::NONE;
    if (tmp < 300) {
        tag = doors.get_door(door_kind).closed;
    } else if (tmp < 399) {
        tag = doors.select_locked_tag(door_kind);
    } else {
        tag = doors.select_jammed_tag(door_kind);
    }

    if (tag == TerrainTag::NONE) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
        return;
    }

    cave_set_feat(player_ptr, pos, tag);
    floor.get_grid(pos).info &= ~(CAVE_MASK);
}
