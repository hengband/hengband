﻿/*!
 * todo ちょっとギリギリ。後で分割を検討する
 * @brief フロア生成時にアイテムを配置する
 * @date 2020/06/01
 * @author Hourier
 */

#include "floor/floor-object.h"
#include "artifact/fixed-art-generator.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/item-getter.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/apply-magic.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "perception/object-perception.h"
#include "system/alloc-entries.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world-object.h"
#include "world/world.h"

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
    describe_flavor(owner_ptr, o_name, o_ptr, 0);
    msg_format_wizard(owner_ptr, CHEAT_OBJECT, _("%sを生成しました。", "%s was generated."), o_name);
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

        object_prep(owner_ptr, j_ptr, k_idx);
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
bool make_gold(player_type *player_ptr, object_type *j_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int i = ((randint1(floor_ptr->object_level + 2) + 2) / 2) - 1;
    if (one_in_(GREAT_OBJ)) {
        i += randint1(floor_ptr->object_level + 1);
    }

    if (coin_type)
        i = coin_type;
    if (i >= MAX_GOLD)
        i = MAX_GOLD - 1;
    object_prep(player_ptr, j_ptr, OBJ_GOLD_LIST + i);

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
    if (!object_is_held_monster(j_ptr)) {
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

    if (object_is_held_monster(j_ptr)) {
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

/*!
 * @brief 生成済のオブジェクトをフロアの所定の位置に落とす。
 * Let an object fall to the ground at or near a location.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param j_ptr 落としたいオブジェクト構造体の参照ポインタ
 * @param chance ドロップの消滅率(%)
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return 生成に成功したらオブジェクトのIDを返す。
 * @details
 * The initial location is assumed to be "in_bounds(floor_ptr, )".\n
 *\n
 * This function takes a parameter "chance".  This is the percentage\n
 * chance that the item will "disappear" instead of drop.  If the object\n
 * has been thrown, then this is the chance of disappearance on contact.\n
 *\n
 * Hack -- this function uses "chance" to determine if it should produce\n
 * some form of "description" of the drop event (under the player).\n
 *\n
 * We check several locations to see if we can find a location at which\n
 * the object can combine, stack, or be placed.  Artifacts will try very\n
 * hard to be placed, including "teleporting" to a useful grid if needed.\n
 */
OBJECT_IDX drop_near(player_type *owner_ptr, object_type *j_ptr, PERCENTAGE chance, POSITION y, POSITION x)
{
    int i, k, d, s;
    POSITION dy, dx;
    POSITION ty, tx = 0;
    OBJECT_IDX o_idx = 0;
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    grid_type *g_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    bool flag = FALSE;
    bool done = FALSE;
#ifdef JP
#else
    bool plural = (j_ptr->number != 1);
#endif
    describe_flavor(owner_ptr, o_name, j_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (!object_is_artifact(j_ptr) && (randint0(100) < chance)) {
#ifdef JP
        msg_format("%sは消えた。", o_name);
#else
        msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
        if (current_world_ptr->wizard)
            msg_print(_("(破損)", "(breakage)"));

        return 0;
    }

    int bs = -1;
    int bn = 0;

    POSITION by = y;
    POSITION bx = x;
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    for (dy = -3; dy <= 3; dy++) {
        for (dx = -3; dx <= 3; dx++) {
            bool comb = FALSE;
            d = (dy * dy) + (dx * dx);
            if (d > 10)
                continue;

            ty = y + dy;
            tx = x + dx;
            if (!in_bounds(floor_ptr, ty, tx))
                continue;
            if (!projectable(owner_ptr, y, x, ty, tx))
                continue;

            g_ptr = &floor_ptr->grid_array[ty][tx];
            if (!cave_drop_bold(floor_ptr, ty, tx))
                continue;

            k = 0;
            for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
                object_type *o_ptr;
                o_ptr = &floor_ptr->o_list[this_o_idx];
                next_o_idx = o_ptr->next_o_idx;
                if (object_similar(o_ptr, j_ptr))
                    comb = TRUE;

                k++;
            }

            if (!comb)
                k++;
            if (k > 99)
                continue;

            s = 1000 - (d + k * 5);
            if (s < bs)
                continue;

            if (s > bs)
                bn = 0;

            if ((++bn >= 2) && !one_in_(bn))
                continue;

            bs = s;
            by = ty;
            bx = tx;

            flag = TRUE;
        }
    }

    if (!flag && !object_is_artifact(j_ptr)) {
#ifdef JP
        msg_format("%sは消えた。", o_name);
#else
        msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
        if (current_world_ptr->wizard)
            msg_print(_("(床スペースがない)", "(no floor space)"));

        return 0;
    }

    for (i = 0; !flag && (i < 1000); i++) {
        ty = rand_spread(by, 1);
        tx = rand_spread(bx, 1);

        if (!in_bounds(floor_ptr, ty, tx))
            continue;

        by = ty;
        bx = tx;

        if (!cave_drop_bold(floor_ptr, by, bx))
            continue;

        flag = TRUE;
    }

    if (!flag) {
        int candidates = 0, pick;
        for (ty = 1; ty < floor_ptr->height - 1; ty++) {
            for (tx = 1; tx < floor_ptr->width - 1; tx++) {
                if (cave_drop_bold(floor_ptr, ty, tx))
                    candidates++;
            }
        }

        if (!candidates) {
#ifdef JP
            msg_format("%sは消えた。", o_name);
#else
            msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif

            if (current_world_ptr->wizard)
                msg_print(_("(床スペースがない)", "(no floor space)"));

            if (preserve_mode) {
                if (object_is_fixed_artifact(j_ptr) && !object_is_known(j_ptr)) {
                    a_info[j_ptr->name1].cur_num = 0;
                }
            }

            return 0;
        }

        pick = randint1(candidates);
        for (ty = 1; ty < floor_ptr->height - 1; ty++) {
            for (tx = 1; tx < floor_ptr->width - 1; tx++) {
                if (cave_drop_bold(floor_ptr, ty, tx)) {
                    pick--;
                    if (!pick)
                        break;
                }
            }

            if (!pick)
                break;
        }

        by = ty;
        bx = tx;
    }

    g_ptr = &floor_ptr->grid_array[by][bx];
    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (object_similar(o_ptr, j_ptr)) {
            object_absorb(o_ptr, j_ptr);
            done = TRUE;
            break;
        }
    }

    if (!done)
        o_idx = o_pop(floor_ptr);

    if (!done && !o_idx) {
#ifdef JP
        msg_format("%sは消えた。", o_name);
#else
        msg_format("The %s disappear%s.", o_name, (plural ? "" : "s"));
#endif
        if (current_world_ptr->wizard)
            msg_print(_("(アイテムが多過ぎる)", "(too many objects)"));

        if (object_is_fixed_artifact(j_ptr)) {
            a_info[j_ptr->name1].cur_num = 0;
        }

        return 0;
    }

    if (!done) {
        object_copy(&floor_ptr->o_list[o_idx], j_ptr);
        j_ptr = &floor_ptr->o_list[o_idx];
        j_ptr->iy = by;
        j_ptr->ix = bx;
        j_ptr->held_m_idx = 0;
        j_ptr->next_o_idx = g_ptr->o_idx;

        g_ptr->o_idx = o_idx;
        done = TRUE;
    }

    note_spot(owner_ptr, by, bx);
    lite_spot(owner_ptr, by, bx);
    sound(SOUND_DROP);

    if (chance && player_bold(owner_ptr, by, bx)) {
        msg_print(_("何かが足下に転がってきた。", "You feel something roll beneath your feet."));
    }

    return o_idx;
}

/*!
 * @brief 床上の魔道具の残り残量メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_charges(floor_type *floor_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &floor_ptr->o_list[item];
    if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND))
        return;
    if (!object_is_known(o_ptr))
        return;

#ifdef JP
    if (o_ptr->pval <= 0) {
        msg_print("この床上のアイテムは、もう魔力が残っていない。");
    } else {
        msg_format("この床上のアイテムは、あと %d 回分の魔力が残っている。", o_ptr->pval);
    }
#else
    if (o_ptr->pval != 1) {
        msg_format("There are %d charges remaining.", o_ptr->pval);
    } else {
        msg_format("There is %d charge remaining.", o_ptr->pval);
    }
#endif
}

/*!
 * @brief 床上のアイテムの残り数メッセージを表示する /
 * Describe the charges on an item on the floor.
 * @param floo_ptr 現在フロアへの参照ポインタ
 * @param item メッセージの対象にしたいアイテム所持スロット
 * @return なし
 */
void floor_item_describe(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->current_floor_ptr->o_list[item];
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, o_ptr, 0);
#ifdef JP
    if (o_ptr->number <= 0) {
        msg_format("床上には、もう%sはない。", o_name);
    } else {
        msg_format("床上には、まだ %sがある。", o_name);
    }
#else
    msg_format("You see %s.", o_name);
#endif
}

/*
 * Choose an item and get auto-picker entry from it.
 */
object_type *choose_object(player_type *owner_ptr, OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, tval_type tval)
{
    OBJECT_IDX item;

    if (idx)
        *idx = INVEN_NONE;

    if (!get_item(owner_ptr, &item, q, s, option, tval))
        return NULL;

    if (idx)
        *idx = item;

    if (item == INVEN_FORCE)
        return NULL;

    return ref_item(owner_ptr, item);
}
