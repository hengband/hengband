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
 * @brief アイテムを定義された基準に従いソートするための関数 /
 * Check if we have space for an item in the pack without overflow
 * @param item1 比較対象アイテムへの参照
 * @param item2 比較対象アイテムへの参照
 * @return item1の方が上位ならばTRUEを返す。
 */
bool object_sort_comp(PlayerType *player_ptr, const ItemEntity &item1, const ItemEntity &item2)
{
    if (!item2.is_valid()) {
        return true;
    }

    const auto item1_tval = item1.bi_key.tval();
    const auto item2_tval = item2.bi_key.tval();
    PlayerRealm pr(player_ptr);
    const auto realm1_book = pr.realm1().get_book();
    const auto realm2_book = pr.realm2().get_book();
    if ((item1_tval == realm1_book) && (item2_tval != realm1_book)) {
        return true;
    }

    if ((item2_tval == realm1_book) && (item1_tval != realm1_book)) {
        return false;
    }

    if ((item1_tval == realm2_book) && (item2_tval != realm2_book)) {
        return true;
    }

    if ((item2_tval == realm2_book) && (item1_tval != realm2_book)) {
        return false;
    }

    if (item1_tval > item2_tval) {
        return true;
    }

    if (item1_tval < item2_tval) {
        return false;
    }

    if (!item1.is_aware()) {
        return false;
    }

    if (!item2.is_aware()) {
        return true;
    }

    const auto item1_sval = item1.bi_key.sval();
    const auto item2_sval = item2.bi_key.sval();
    if (item1_sval < item2_sval) {
        return true;
    }

    if (item1_sval > item2_sval) {
        return false;
    }

    if (!item1.is_known()) {
        return false;
    }

    if (!item2.is_known()) {
        return true;
    }

    const auto item1_rank = get_item_sort_rank(item1);
    const auto item2_rank = get_item_sort_rank(item2);
    if (item1_rank < item2_rank) {
        return true;
    }

    if (item1_rank > item2_rank) {
        return false;
    }

    switch (item1_tval) {
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::MONSTER_REMAINS:
    case ItemKindType::CAPTURE: {
        const auto &monrace1 = item1.get_monrace();
        const auto &monrace2 = item2.get_monrace();
        if (monrace2.order_level_strictly(monrace1)) {
            return true;
        }

        if ((monrace1.level == monrace2.level) && (item1.pval < item2.pval)) {
            return true;
        }

        return false;
    }
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        if (item1.to_h + item1.to_d < item2.to_h + item2.to_d) {
            return true;
        }

        if (item1.to_h + item1.to_d > item2.to_h + item2.to_d) {
            return false;
        }

        break;
    case ItemKindType::ROD:
        if (item1.pval < item2.pval) {
            return true;
        }

        if (item1.pval > item2.pval) {
            return false;
        }

        break;
    default:
        break;
    }

    return item1.calc_price() > item2.calc_price();
}
