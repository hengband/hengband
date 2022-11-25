/*!
 * @brief オブジェクトのソート処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "util/object-sort.h"
#include "monster-race/monster-race.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player/player-realm.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

static int get_item_sort_rank(const ItemEntity &item)
{
    if (item.is_fixed_artifact()) {
        return 3;
    }

    if (item.art_name) {
        return 2;
    }

    if (item.is_ego()) {
        return 1;
    }

    return 0;
}

/*!
 * @brief オブジェクトを定義された基準に従いソートするための関数 /
 * Check if we have space for an item in the pack without overflow
 * @param o_ptr 比較対象オブジェクトの構造体参照ポインタ1
 * @param o_value o_ptrのアイテム価値（手動であらかじめ代入する必要がある？）
 * @param j_ptr 比較対象オブジェクトの構造体参照ポインタ2
 * @return o_ptrの方が上位ならばTRUEを返す。
 */
bool object_sort_comp(PlayerType *player_ptr, ItemEntity *o_ptr, int32_t o_value, ItemEntity *j_ptr)
{
    if (j_ptr->bi_id == 0) {
        return true;
    }

    const auto o_tval = o_ptr->tval;
    const auto j_tval = j_ptr->tval;
    if ((o_tval == get_realm1_book(player_ptr)) && (j_tval != get_realm1_book(player_ptr))) {
        return true;
    }

    if ((j_tval == get_realm1_book(player_ptr)) && (o_tval != get_realm1_book(player_ptr))) {
        return false;
    }

    if ((o_tval == get_realm2_book(player_ptr)) && (j_tval != get_realm2_book(player_ptr))) {
        return true;
    }

    if ((j_tval == get_realm2_book(player_ptr)) && (o_tval != get_realm2_book(player_ptr))) {
        return false;
    }

    if (o_tval > j_tval) {
        return true;
    }

    if (o_tval < j_tval) {
        return false;
    }

    if (!o_ptr->is_aware()) {
        return false;
    }

    if (!j_ptr->is_aware()) {
        return true;
    }

    const auto o_sval = o_ptr->sval;
    const auto j_sval = j_ptr->sval;
    if (o_sval < j_sval) {
        return true;
    }

    if (o_sval > j_sval) {
        return false;
    }

    if (!o_ptr->is_known()) {
        return false;
    }

    if (!j_ptr->is_known()) {
        return true;
    }

    const auto o_rank = get_item_sort_rank(*o_ptr);
    const auto j_rank = get_item_sort_rank(*j_ptr);
    if (o_rank < j_rank) {
        return true;
    }

    if (o_rank > j_rank) {
        return false;
    }

    switch (o_tval) {
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::CORPSE:
    case ItemKindType::CAPTURE: {
        auto o_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        auto j_r_idx = i2enum<MonsterRaceId>(j_ptr->pval);
        if (monraces_info[o_r_idx].level < monraces_info[j_r_idx].level) {
            return true;
        }

        if ((monraces_info[o_r_idx].level == monraces_info[j_r_idx].level) && (o_ptr->pval < j_ptr->pval)) {
            return true;
        }

        return false;
    }
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        if (o_ptr->to_h + o_ptr->to_d < j_ptr->to_h + j_ptr->to_d) {
            return true;
        }

        if (o_ptr->to_h + o_ptr->to_d > j_ptr->to_h + j_ptr->to_d) {
            return false;
        }

        break;
    case ItemKindType::ROD:
        if (o_ptr->pval < j_ptr->pval) {
            return true;
        }

        if (o_ptr->pval > j_ptr->pval) {
            return false;
        }

        break;
    default:
        break;
    }

    return o_value > j_ptr->get_price();
}
