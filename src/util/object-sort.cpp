/*!
 * @brief オブジェクトのソート処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "util/object-sort.h"
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

    if (item.is_random_artifact()) {
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
    if (!j_ptr->is_valid()) {
        return true;
    }

    const auto o_tval = o_ptr->bi_key.tval();
    const auto j_tval = j_ptr->bi_key.tval();
    PlayerRealm pr(player_ptr);
    const auto realm1_book = pr.realm1().get_book();
    const auto realm2_book = pr.realm2().get_book();
    if ((o_tval == realm1_book) && (j_tval != realm1_book)) {
        return true;
    }

    if ((j_tval == realm1_book) && (o_tval != realm1_book)) {
        return false;
    }

    if ((o_tval == realm2_book) && (j_tval != realm2_book)) {
        return true;
    }

    if ((j_tval == realm2_book) && (o_tval != realm2_book)) {
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

    const auto o_sval = o_ptr->bi_key.sval();
    const auto j_sval = j_ptr->bi_key.sval();
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
    case ItemKindType::MONSTER_REMAINS:
    case ItemKindType::CAPTURE: {
        const auto &monrace1 = o_ptr->get_monrace();
        const auto &monrace2 = j_ptr->get_monrace();
        if (monrace2.order_level_strictly(monrace1)) {
            return true;
        }

        if ((monrace1.level == monrace2.level) && (o_ptr->pval < j_ptr->pval)) {
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
