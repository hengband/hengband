#include "grid/object-placer.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "world/world-object.h"

/*!
 * @brief フロアの指定位置に生成階に応じた財宝オブジェクトの生成を行う。
 * Places a treasure (Gold or Gems) at given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
void place_gold(player_type *player_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (!in_bounds(floor_ptr, y, x))
        return;
    if (!cave_drop_bold(floor_ptr, y, x))
        return;
    if (g_ptr->o_idx)
        return;

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_wipe(q_ptr);
    if (!make_gold(player_ptr, q_ptr))
        return;

    OBJECT_IDX o_idx = o_pop(floor_ptr);
    if (o_idx == 0)
        return;

    object_type *o_ptr;
    o_ptr = &floor_ptr->o_list[o_idx];
    object_copy(o_ptr, q_ptr);

    o_ptr->iy = y;
    o_ptr->ix = x;
    o_ptr->next_o_idx = g_ptr->o_idx;

    g_ptr->o_idx = o_idx;
    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
}

/*!
 * @brief フロアの指定位置に生成階に応じたベースアイテムの生成を行う。
 * Attempt to place an object (normal or good/great) at the given location.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "object_level" for the "generation level".\n
 * This routine requires a clean floor grid destination.\n
 */
void place_object(player_type *owner_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    object_type forge;
    object_type *q_ptr;
    if (!in_bounds(floor_ptr, y, x) || !cave_drop_bold(floor_ptr, y, x) || (g_ptr->o_idx != 0))
        return;

    q_ptr = &forge;
    object_wipe(q_ptr);
    if (!make_object(owner_ptr, q_ptr, mode))
        return;

    OBJECT_IDX o_idx = o_pop(floor_ptr);
    if (o_idx == 0) {
        if (object_is_fixed_artifact(q_ptr)) {
            a_info[q_ptr->name1].cur_num = 0;
        }

        return;
    }

    object_type *o_ptr;
    o_ptr = &floor_ptr->o_list[o_idx];
    object_copy(o_ptr, q_ptr);

    o_ptr->iy = y;
    o_ptr->ix = x;
    o_ptr->next_o_idx = g_ptr->o_idx;

    g_ptr->o_idx = o_idx;
    note_spot(owner_ptr, y, x);
    lite_spot(owner_ptr, y, x);
}
