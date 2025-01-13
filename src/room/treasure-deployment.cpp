/*!
 * @brief 部屋にアイテム・モンスター・罠を配置する処理
 * @date 2020/07/24
 * @author Hourier
 */

#include "room/treasure-deployment.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-apply-magic.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include <cstdlib>

/*
 * Routine that fills the empty areas of a room with treasure and monsters.
 */
void fill_treasure(PlayerType *player_ptr, const Pos2D &top_left, const Pos2D &bottom_right, int difficulty)
{
    const auto center = top_left.centered(bottom_right);
    const auto size = std::abs(bottom_right.x - top_left.x) + std::abs(bottom_right.y - top_left.y);
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto x = top_left.x; x <= bottom_right.x; x++) {
        for (auto y = top_left.y; y <= bottom_right.y; y++) {
            const Pos2D pos(y, x);
            auto value = distance(center.x, center.y, pos.x, pos.y) * 100 / size + randint1(10) - difficulty;
            if ((randint1(100) - difficulty * 3) > 50) {
                value = 20;
            }

            const auto has_terrain_place = floor.has_terrain_characteristics(pos, TerrainCharacteristics::PLACE);
            const auto has_terrain_drop = floor.has_terrain_characteristics(pos, TerrainCharacteristics::DROP);
            if (!floor.get_grid(pos).is_floor() && (!has_terrain_place || !has_terrain_drop)) {
                continue;
            }

            if (value < 0) {
                floor.monster_level = floor.base_level + 40;
                place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                floor.monster_level = floor.base_level;
                floor.object_level = floor.base_level + 20;
                place_object(player_ptr, pos.y, pos.x, AM_GOOD);
                floor.object_level = floor.base_level;
                continue;
            }

            if (value < 5) {
                floor.monster_level = floor.base_level + 20;
                place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                floor.monster_level = floor.base_level;
                floor.object_level = floor.base_level + 10;
                place_object(player_ptr, pos.y, pos.x, AM_GOOD);
                floor.object_level = floor.base_level;
            }

            if (value < 10) {
                floor.monster_level = floor.base_level + 9;
                place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                floor.monster_level = floor.base_level;
                continue;
            }

            if (value < 17) {
                continue;
            }

            if (value < 23) {
                if (one_in_(4)) {
                    place_object(player_ptr, pos.y, pos.x, 0L);
                    continue;
                }

                place_trap(&floor, y, x);
                continue;
            }

            if (value < 30) {
                floor.monster_level = floor.base_level + 5;
                place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                floor.monster_level = floor.base_level;
                place_trap(&floor, y, x);
                continue;
            }

            if (value < 40) {
                if (one_in_(2)) {
                    floor.monster_level = floor.base_level + 3;
                    place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                    floor.monster_level = floor.base_level;
                }

                if (one_in_(2)) {
                    floor.object_level = floor.base_level + 7;
                    place_object(player_ptr, pos.y, pos.x, 0L);
                    floor.object_level = floor.base_level;
                }

                continue;
            }

            if (value < 50) {
                place_trap(&floor, y, x);
                continue;
            }

            if (one_in_(5)) {
                place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
                continue;
            }

            if (one_in_(2)) {
                place_trap(&floor, y, x);
                continue;
            }

            if (one_in_(2)) {
                place_object(player_ptr, pos.y, pos.x, 0L);
            }

            continue;
        }
    }
}
