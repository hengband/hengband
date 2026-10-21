/*!
 * @brief 魔道具の充填施設のテスト
 *
 * まとめて充填するときの料金が、高価な杖・魔法棒でもあふれないことを検証する。
 */

#include "market/building-recharger.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "test/scoped-vector-wrapper.h"

#include <cstdint>
#include <doctest/doctest.h>

namespace {
/*!
 * @brief ベースアイテムを1つだけ登録し、そのアイテムを作る
 * @param tval アイテムの種別
 * @param level ベースアイテムのレベル
 * @param pval ベースアイテムの回数・充填の時間
 * @param cost ベースアイテムの価格
 * @return 登録したベースアイテムのアイテム (1個、回数は0)
 */
ItemEntity make_item(ItemKindType tval, int level, short pval, int cost)
{
    auto &items = BaseitemList::get_instance();
    items.resize(2);
    auto &base = items.get_baseitem(1);
    base.bi_key = BaseitemKey(tval, 0);
    base.level = level;
    base.pval = pval;
    base.cost = cost;
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = base.bi_key;
    item.number = 1;
    return item;
}
}

TEST_CASE("calc_recharge_all_price() keeps the price for ordinary devices")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };

    SUBCASE("rod")
    {
        auto item = make_item(ItemKindType::ROD, 20, 50, 1000);
        item.timeout = 100;
        CHECK(calc_recharge_all_price(item) == 2000);
    }

    SUBCASE("staff with the minimum price per charge")
    {
        auto item = make_item(ItemKindType::STAFF, 20, 10, 50);
        item.pval = 4;
        CHECK(calc_recharge_all_price(item) == 60);
    }

    SUBCASE("wands")
    {
        auto item = make_item(ItemKindType::WAND, 20, 10, 1000);
        item.number = 2;
        item.pval = 5;
        CHECK(calc_recharge_all_price(item) == 1500);
    }
}

TEST_CASE("calc_recharge_all_price() does not overflow for expensive devices")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };

    SUBCASE("staffs")
    {
        auto item = make_item(ItemKindType::STAFF, 20, 30, 9999999);
        item.number = 99;
        CHECK(calc_recharge_all_price(item) == int64_t{ 2969997030 });
    }

    SUBCASE("wands")
    {
        auto item = make_item(ItemKindType::WAND, 20, 100, 9999999);
        item.number = 99;
        CHECK(calc_recharge_all_price(item) == int64_t{ 9899990100 });
    }
}
