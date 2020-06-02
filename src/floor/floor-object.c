﻿/*!
 * @brief フロア生成時にアイテムを配置する
 * @date 2020/06/01
 * @author Hourier
 */

#include "floor/floor-object.h"
#include "object/artifact.h"
#include "object/item-apply-magic.h"
#include "object/object-appraiser.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-hook.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object2.h"
#include "object/special-object-flags.h"
#include "view/object-describer.h"

#define MAX_GOLD 18 /* Number of "gold" entries */

/*!
 * @brief オブジェクト生成テーブルに生成制約を加える /
 * Apply a "object restriction function" to the "object allocation table"
 * @return 常に0を返す。
 * @details 生成の制約はグローバルのget_obj_num_hook関数ポインタで加える
 */
static errr get_obj_num_prep(void)
{
    alloc_entry *table = alloc_kind_table;
    for (OBJECT_IDX i = 0; i < alloc_kind_size; i++) {
        if (!get_obj_num_hook || (*get_obj_num_hook)(table[i].index)) {
            table[i].prob2 = table[i].prob1;
        } else {
            table[i].prob2 = 0;
        }
    }

    return 0;
}

/*!
 * @brief デバッグ時にアイテム生成情報をメッセージに出力する / Cheat -- describe a created object for the user
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr デバッグ出力するオブジェクトの構造体参照ポインタ
 * @return なし
 */
static void object_mention(player_type *owner_ptr, object_type *o_ptr)
{
    object_aware(owner_ptr, o_ptr);
    object_known(o_ptr);

    o_ptr->ident |= (IDENT_FULL_KNOWN);
    GAME_TEXT o_name[MAX_NLEN];
    object_desc(owner_ptr, o_name, o_ptr, 0);
    msg_format_wizard(CHEAT_OBJECT, _("%sを生成しました。", "%s was generated."), o_name);
}

/*!
 * @brief 生成階に応じたベースアイテムの生成を行う。
 * Attempt to make an object (normal or good/great)
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @param mode オプションフラグ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * This routine plays nasty games to generate the "special artifacts".\n
 * This routine uses "floor_ptr->object_level" for the "generation level".\n
 * We assume that the given object has been "wiped".\n
 */
bool make_object(player_type *owner_ptr, object_type *j_ptr, BIT_FLAGS mode)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    PERCENTAGE prob = ((mode & AM_GOOD) ? 10 : 1000);
    DEPTH base = ((mode & AM_GOOD) ? (floor_ptr->object_level + 10) : floor_ptr->object_level);
    if (!one_in_(prob) || !make_artifact_special(owner_ptr, j_ptr)) {
        KIND_OBJECT_IDX k_idx;
        if ((mode & AM_GOOD) && !get_obj_num_hook) {
            get_obj_num_hook = kind_is_good;
        }

        if (get_obj_num_hook)
            get_obj_num_prep();

        k_idx = get_obj_num(owner_ptr, base, mode);
        if (get_obj_num_hook) {
            get_obj_num_hook = NULL;
            get_obj_num_prep();
        }

        if (!k_idx)
            return FALSE;

        object_prep(j_ptr, k_idx);
    }

    apply_magic(owner_ptr, j_ptr, floor_ptr->object_level, mode);
    switch (j_ptr->tval) {
    case TV_SPIKE:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT: {
        if (!j_ptr->name1)
            j_ptr->number = (byte)damroll(6, 7);
    }
    }

    if (cheat_peek)
        object_mention(owner_ptr, j_ptr);

    return TRUE;
}

/*!
 * @brief 生成階に応じた財宝オブジェクトの生成を行う。
 * Make a treasure object
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @param j_ptr 生成結果を収めたいオブジェクト構造体の参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * The location must be a legal, clean, floor grid.
 */
bool make_gold(floor_type *floor_ptr, object_type *j_ptr)
{
    int i = ((randint1(floor_ptr->object_level + 2) + 2) / 2) - 1;
    if (one_in_(GREAT_OBJ)) {
        i += randint1(floor_ptr->object_level + 1);
    }

    if (coin_type)
        i = coin_type;
    if (i >= MAX_GOLD)
        i = MAX_GOLD - 1;
    object_prep(j_ptr, OBJ_GOLD_LIST + i);

    s32b base = k_info[OBJ_GOLD_LIST + i].cost;
    j_ptr->pval = (base + (8L * randint1(base)) + randint1(8));

    return TRUE;
}

/*!
 * @brief フロア中のアイテムを全て削除する / Deletes all objects at given location
 * Delete a dungeon object
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 削除したフロアマスのY座標
 * @param x 削除したフロアマスのX座標
 * @return なし
 */
void delete_all_items_from_floor(player_type *player_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return;

    g_ptr = &floor_ptr->grid_array[y][x];
    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        object_wipe(o_ptr);
        floor_ptr->o_cnt--;
    }

    g_ptr->o_idx = 0;
    lite_spot(player_ptr, y, x);
}

/*!
 * @brief 床上のアイテムの数を増やす /
 * Increase the "number" of an item on the floor
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param item 増やしたいアイテムの所持スロット
 * @param num 増やしたいアイテムの数
 * @return なし
 */
void floor_item_increase(floor_type *floor_ptr, INVENTORY_IDX item, ITEM_NUMBER num)
{
    object_type *o_ptr = &floor_ptr->o_list[item];
    num += o_ptr->number;
    if (num > 255)
        num = 255;
    else if (num < 0)
        num = 0;

    num -= o_ptr->number;
    o_ptr->number += num;
}

/*!
 * @brief 床上の数の無くなったアイテムスロットを消去する /
 * Optimize an item on the floor (destroy "empty" items)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param item 消去したいアイテムの所持スロット
 * @return なし
 */
void floor_item_optimize(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->current_floor_ptr->o_list[item];
    if (!o_ptr->k_idx)
        return;
    if (o_ptr->number)
        return;

    delete_object_idx(owner_ptr, item);
}

/*!
 * @brief オブジェクトを削除する /
 * Delete a dungeon object
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 * @return なし
 * @details
 * Handle "stacks" of objects correctly.
 */
void delete_object_idx(player_type *player_ptr, OBJECT_IDX o_idx)
{
    object_type *j_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    excise_object_idx(floor_ptr, o_idx);
    j_ptr = &floor_ptr->o_list[o_idx];
    if (!OBJECT_IS_HELD_MONSTER(j_ptr)) {
        POSITION y, x;
        y = j_ptr->iy;
        x = j_ptr->ix;
        lite_spot(player_ptr, y, x);
    }

    object_wipe(j_ptr);
    floor_ptr->o_cnt--;
}

/*!
 * @brief 床上、モンスター所持でスタックされたアイテムを削除しスタックを補完する / Excise a dungeon object from any stacks
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param o_idx 削除対象のオブジェクト構造体ポインタ
 * @return なし
 */
void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx)
{
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    OBJECT_IDX prev_o_idx = 0;
    object_type *j_ptr;
    j_ptr = &floor_ptr->o_list[o_idx];

    if (OBJECT_IS_HELD_MONSTER(j_ptr)) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[j_ptr->held_m_idx];
        for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx) {
            object_type *o_ptr;
            o_ptr = &floor_ptr->o_list[this_o_idx];
            next_o_idx = o_ptr->next_o_idx;
            if (this_o_idx != o_idx) {
                prev_o_idx = this_o_idx;
                continue;
            }

            if (prev_o_idx == 0) {
                m_ptr->hold_o_idx = next_o_idx;
            } else {
                object_type *k_ptr;
                k_ptr = &floor_ptr->o_list[prev_o_idx];
                k_ptr->next_o_idx = next_o_idx;
            }

            o_ptr->next_o_idx = 0;
            break;
        }

        return;
    }

    grid_type *g_ptr;
    POSITION y = j_ptr->iy;
    POSITION x = j_ptr->ix;
    g_ptr = &floor_ptr->grid_array[y][x];
    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (this_o_idx != o_idx) {
            prev_o_idx = this_o_idx;
            continue;
        }

        if (prev_o_idx == 0) {
            g_ptr->o_idx = next_o_idx;
        } else {
            object_type *k_ptr;
            k_ptr = &floor_ptr->o_list[prev_o_idx];
            k_ptr->next_o_idx = next_o_idx;
        }

        o_ptr->next_o_idx = 0;
        break;
    }
}
