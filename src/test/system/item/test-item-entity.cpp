#include "object-enchant/object-ego.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/item/item-entity.h"
#include "test/scoped-vector-wrapper.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>
#include <limits>
#include <utility>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("ItemEntity calc_price discounts large prices without multiplication overflow")
{
    auto saved_egos = std::move(egos_info);
    const auto restore = util::make_finalizer([&saved_egos] { egos_info.swap(saved_egos); });
    egos_info.clear();
    auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
    ego.cost = 12345;
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    test::ScopedVectorWrapper<BaseitemRecords> saved_records{ BaseitemRecords::get_instance() };
    BaseitemRecords::get_instance().initialize(2);
    auto &items = BaseitemList::get_instance();
    items.resize(2);
    auto &base = items.get_baseitem(1);
    base.name = "Test";
    base.bi_key = BaseitemKey(ItemKindType::RING, 1);
    base.cost = 99999999;
    ego.cost = 67108863;
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = base.bi_key;
    item.ego_idx = EgoType::A_MORGUL;
    item.mark_as_known();
    item.discount = 50;
    CHECK(item.calc_price() == 83554431);
    item.discount = 90;
    CHECK(item.calc_price() == 16710887);
    base.cost = 50;
    ego.cost = 100;
    item.discount = 50;
    CHECK(item.calc_price() == 75);
}
