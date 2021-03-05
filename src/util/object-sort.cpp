﻿/*!
 * @brief オブジェクトのソート処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "util/object-sort.h"
#include "monster-race/monster-race.h"
#include "object-hook/hook-enchant.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/player-realm.h"

/*!
 * @brief オブジェクトを定義された基準に従いソートするための関数 /
 * Check if we have space for an item in the pack without overflow
 * @param o_ptr 比較対象オブジェクトの構造体参照ポインタ1
 * @param o_value o_ptrのアイテム価値（手動であらかじめ代入する必要がある？）
 * @param j_ptr 比較対象オブジェクトの構造体参照ポインタ2
 * @return o_ptrの方が上位ならばTRUEを返す。
 */
bool object_sort_comp(player_type *player_ptr, object_type *o_ptr, s32b o_value, object_type *j_ptr)
{
    int o_type, j_type;
    if (!j_ptr->k_idx)
        return TRUE;

    if ((o_ptr->tval == get_realm1_book(player_ptr)) && (j_ptr->tval != get_realm1_book(player_ptr)))
        return TRUE;
    if ((j_ptr->tval == get_realm1_book(player_ptr)) && (o_ptr->tval != get_realm1_book(player_ptr)))
        return FALSE;

    if ((o_ptr->tval == get_realm2_book(player_ptr)) && (j_ptr->tval != get_realm2_book(player_ptr)))
        return TRUE;
    if ((j_ptr->tval == get_realm2_book(player_ptr)) && (o_ptr->tval != get_realm2_book(player_ptr)))
        return FALSE;

    if (o_ptr->tval > j_ptr->tval)
        return TRUE;
    if (o_ptr->tval < j_ptr->tval)
        return FALSE;

    if (!object_is_aware(o_ptr))
        return FALSE;
    if (!object_is_aware(j_ptr))
        return TRUE;

    if (o_ptr->sval < j_ptr->sval)
        return TRUE;
    if (o_ptr->sval > j_ptr->sval)
        return FALSE;

    if (!object_is_known(o_ptr))
        return FALSE;
    if (!object_is_known(j_ptr))
        return TRUE;

    if (object_is_fixed_artifact(o_ptr))
        o_type = 3;
    else if (o_ptr->art_name)
        o_type = 2;
    else if (object_is_ego(o_ptr))
        o_type = 1;
    else
        o_type = 0;

    if (object_is_fixed_artifact(j_ptr))
        j_type = 3;
    else if (j_ptr->art_name)
        j_type = 2;
    else if (object_is_ego(j_ptr))
        j_type = 1;
    else
        j_type = 0;

    if (o_type < j_type)
        return TRUE;
    if (o_type > j_type)
        return FALSE;

    switch (o_ptr->tval) {
    case TV_FIGURINE:
    case TV_STATUE:
    case TV_CORPSE:
    case TV_CAPTURE:
        if (r_info[o_ptr->pval].level < r_info[j_ptr->pval].level)
            return TRUE;
        if ((r_info[o_ptr->pval].level == r_info[j_ptr->pval].level) && (o_ptr->pval < j_ptr->pval))
            return TRUE;
        return FALSE;

    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
        if (o_ptr->to_h + o_ptr->to_d < j_ptr->to_h + j_ptr->to_d)
            return TRUE;
        if (o_ptr->to_h + o_ptr->to_d > j_ptr->to_h + j_ptr->to_d)
            return FALSE;
        break;

    case TV_ROD:
        if (o_ptr->pval < j_ptr->pval)
            return TRUE;
        if (o_ptr->pval > j_ptr->pval)
            return FALSE;
        break;
    }

    return o_value > object_value(player_ptr, j_ptr);
}
