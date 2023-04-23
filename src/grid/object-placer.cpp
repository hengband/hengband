#include "grid/object-placer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "world/world-object.h"

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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (!in_bounds(floor_ptr, y, x)) {
        return;
    }
    if (!cave_drop_bold(floor_ptr, y, x)) {
        return;
    }
    if (!g_ptr->o_idx_list.empty()) {
        return;
    }

    ItemEntity forge;
    ItemEntity *q_ptr;
    q_ptr = &forge;
    q_ptr->wipe();
    if (!make_gold(player_ptr, q_ptr)) {
        return;
    }

    OBJECT_IDX o_idx = o_pop(floor_ptr);
    if (o_idx == 0) {
        return;
    }

    ItemEntity *o_ptr;
    o_ptr = &floor_ptr->o_list[o_idx];
    o_ptr->copy_from(q_ptr);

    o_ptr->iy = y;
    o_ptr->ix = x;
    g_ptr->o_idx_list.add(floor_ptr, o_idx);

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    ItemEntity forge;
    ItemEntity *q_ptr;
    if (!in_bounds(floor_ptr, y, x) || !cave_drop_bold(floor_ptr, y, x) || !g_ptr->o_idx_list.empty()) {
        return;
    }

    q_ptr = &forge;
    q_ptr->wipe();
    if (!make_object(player_ptr, q_ptr, mode)) {
        return;
    }

    OBJECT_IDX o_idx = o_pop(floor_ptr);
    if (o_idx == 0) {
        if (q_ptr->is_fixed_artifact()) {
            ArtifactsInfo::get_instance().get_artifact(q_ptr->fixed_artifact_idx).is_generated = false;
        }

        return;
    }

    ItemEntity *o_ptr;
    o_ptr = &floor_ptr->o_list[o_idx];
    o_ptr->copy_from(q_ptr);

    o_ptr->iy = y;
    o_ptr->ix = x;
    g_ptr->o_idx_list.add(floor_ptr, o_idx);

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
}
