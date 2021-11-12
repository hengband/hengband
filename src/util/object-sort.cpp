/*!
 * @brief オブジェクトのソート処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "util/object-sort.h"
#include "monster-race/monster-race.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/player-realm.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief オブジェクトを定義された基準に従いソートするための関数 /
 * Check if we have space for an item in the pack without overflow
 * @param o_ptr 比較対象オブジェクトの構造体参照ポインタ1
 * @param o_value o_ptrのアイテム価値（手動であらかじめ代入する必要がある？）
 * @param j_ptr 比較対象オブジェクトの構造体参照ポインタ2
 * @return o_ptrの方が上位ならばTRUEを返す。
 */
bool object_sort_comp(PlayerType *player_ptr, object_type *o_ptr, int32_t o_value, object_type *j_ptr)
{
    int o_type, j_type;
    if (!j_ptr->k_idx)
        return true;

    if ((o_ptr->tval == get_realm1_book(player_ptr)) && (j_ptr->tval != get_realm1_book(player_ptr)))
        return true;
    if ((j_ptr->tval == get_realm1_book(player_ptr)) && (o_ptr->tval != get_realm1_book(player_ptr)))
        return false;

    if ((o_ptr->tval == get_realm2_book(player_ptr)) && (j_ptr->tval != get_realm2_book(player_ptr)))
        return true;
    if ((j_ptr->tval == get_realm2_book(player_ptr)) && (o_ptr->tval != get_realm2_book(player_ptr)))
        return false;

    if (o_ptr->tval > j_ptr->tval)
        return true;
    if (o_ptr->tval < j_ptr->tval)
        return false;

    if (!o_ptr->is_aware())
        return false;
    if (!j_ptr->is_aware())
        return true;

    if (o_ptr->sval < j_ptr->sval)
        return true;
    if (o_ptr->sval > j_ptr->sval)
        return false;

    if (!o_ptr->is_known())
        return false;
    if (!j_ptr->is_known())
        return true;

    if (o_ptr->is_fixed_artifact())
        o_type = 3;
    else if (o_ptr->art_name)
        o_type = 2;
    else if (o_ptr->is_ego())
        o_type = 1;
    else
        o_type = 0;

    if (j_ptr->is_fixed_artifact())
        j_type = 3;
    else if (j_ptr->art_name)
        j_type = 2;
    else if (j_ptr->is_ego())
        j_type = 1;
    else
        j_type = 0;

    if (o_type < j_type)
        return true;
    if (o_type > j_type)
        return false;

    switch (o_ptr->tval) {
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::CORPSE:
    case ItemKindType::CAPTURE:
        if (r_info[o_ptr->pval].level < r_info[j_ptr->pval].level)
            return true;
        if ((r_info[o_ptr->pval].level == r_info[j_ptr->pval].level) && (o_ptr->pval < j_ptr->pval))
            return true;
        return false;

    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        if (o_ptr->to_h + o_ptr->to_d < j_ptr->to_h + j_ptr->to_d)
            return true;
        if (o_ptr->to_h + o_ptr->to_d > j_ptr->to_h + j_ptr->to_d)
            return false;
        break;

    case ItemKindType::ROD:
        if (o_ptr->pval < j_ptr->pval)
            return true;
        if (o_ptr->pval > j_ptr->pval)
            return false;
        break;

    default:
        break;
    }

    return o_value > object_value(j_ptr);
}
