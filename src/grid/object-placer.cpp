#include "grid/object-placer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "system/artifact-type-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"

/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う。
 * Places a treasure (Gold or Gems) at given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
void place_gold(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    if (!in_bounds(floor, pos.y, pos.x)) {
        return;
    }
    if (!cave_drop_bold(floor, pos.y, pos.x)) {
        return;
    }
    if (!grid.o_idx_list.empty()) {
        return;
    }

    auto item = floor.make_gold();
    const auto item_idx = floor.pop_empty_index_item();
    if (item_idx == 0) {
        return;
    }

    item.iy = pos.y;
    item.ix = pos.x;
    floor.o_list[item_idx] = std::move(item);
    grid.o_idx_list.add(floor, item_idx);

    note_spot(player_ptr, pos.y, pos.x);
    lite_spot(player_ptr, pos.y, pos.x);
}

/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う。
 * Attempt to place an object (normal or good/great) at the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "object_level" for the "generation level".\n
 * This routine requires a clean floor grid destination.\n
 */
void place_object(PlayerType *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    if (!in_bounds(floor, y, x) || !cave_drop_bold(floor, y, x) || !grid.o_idx_list.empty()) {
        return;
    }

    const auto item_idx = floor.pop_empty_index_item();
    if (item_idx == 0) {
        return;
    }

    auto &item = floor.o_list[item_idx];
    item.wipe();
    if (!make_object(player_ptr, &item, mode)) {
        return;
    }

    item.iy = pos.y;
    item.ix = pos.x;
    grid.o_idx_list.add(floor, item_idx);

    note_spot(player_ptr, pos.y, pos.x);
    lite_spot(player_ptr, pos.y, pos.x);
}
