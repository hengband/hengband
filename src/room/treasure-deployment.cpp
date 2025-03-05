/*!
 * @brief 部屋にアイテム・モンスター・罠を配置する処理
 * @date 2020/07/24
 * @author Hourier
 */

#include "room/treasure-deployment.h"
#include "floor/cave.h"
#include "grid/object-placer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-apply-magic.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include <cstdlib>

namespace {
void deploy_treasure(PlayerType *player_ptr, FloorType &floor, const Pos2D &center, const Pos2D &pos, int size, int difficulty)
{
    auto value = Grid::calc_distance(center, pos) * 100 / size + randint1(10) - difficulty;

    /// @note
    /// v2.2.1のコードのコメントを見ると強制的に空白マスに設定するのを意図しているようだが、
    /// 後のコードを見ればわかるように本来は value に10以上17未満を設定する必要があると思われる。
    /// 少なくともv2.2.1の段階ですでにこうなっているので、最初から間違っていた可能性が高い。
    /// ゲームバランスに影響があるため修正するかは要検討。
    if ((randint1(100) - difficulty * 3) > 50) {
        value = 20;
    }

    const auto has_terrain_place = floor.has_terrain_characteristics(pos, TerrainCharacteristics::PLACE);
    const auto has_terrain_drop = floor.has_terrain_characteristics(pos, TerrainCharacteristics::DROP);
    if (!floor.get_grid(pos).is_floor() && (!has_terrain_place || !has_terrain_drop)) {
        return;
    }

    if (value < 0) {
        floor.monster_level = floor.base_level + 40;
        place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
        floor.monster_level = floor.base_level;
        floor.object_level = floor.base_level + 20;
        place_object(player_ptr, pos, AM_GOOD);
        floor.object_level = floor.base_level;
        return;
    }

    if (value < 5) {
        floor.monster_level = floor.base_level + 20;
        place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
        floor.monster_level = floor.base_level;
        floor.object_level = floor.base_level + 10;
        place_object(player_ptr, pos, AM_GOOD);
        floor.object_level = floor.base_level;
        return;
    }

    if (value < 10) {
        floor.monster_level = floor.base_level + 9;
        place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
        floor.monster_level = floor.base_level;
        return;
    }

    if (value < 17) {
        // 意図的になにも設置せず空白マスとする
        return;
    }

    if (value < 23) {
        if (one_in_(4)) {
            place_object(player_ptr, pos, 0);
            return;
        }
        floor.place_trap_at(pos);
        return;
    }

    if (value < 30) {
        floor.monster_level = floor.base_level + 5;
        place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
        floor.monster_level = floor.base_level;
        floor.place_trap_at(pos);
        return;
    }

    if (value < 40) {
        if (one_in_(2)) {
            floor.monster_level = floor.base_level + 3;
            place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
            floor.monster_level = floor.base_level;
        }
        if (one_in_(2)) {
            floor.object_level = floor.base_level + 7;
            place_object(player_ptr, pos, 0);
            floor.object_level = floor.base_level;
        }
        return;
    }

    if (value < 50) {
        floor.place_trap_at(pos);
        return;
    }

    if (one_in_(5)) {
        place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
        return;
    }

    if (one_in_(2)) {
        floor.place_trap_at(pos);
        return;
    }

    if (one_in_(2)) {
        place_object(player_ptr, pos, 0);
    }
}
}

/*
 * Routine that fills the empty areas of a room with treasure and monsters.
 */
void fill_treasure(PlayerType *player_ptr, const Rect2D &area, int difficulty)
{
    const auto center = area.center();
    const auto size = area.width() - 1 + area.height() - 1;
    auto &floor = *player_ptr->current_floor_ptr;

    for (const auto &pos : area) {
        deploy_treasure(player_ptr, floor, center, pos, size, difficulty);
    }
}
