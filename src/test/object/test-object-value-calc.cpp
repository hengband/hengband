/*!
 * @brief アイテムの特性の価値の計算 (object/object-value-calc.cpp) のテスト
 *
 * 祝福の価値が、善の神の祝福でも邪の神の祝福でも同じであることを確かめる。
 */

#include "object-enchant/tr-types.h"
#include "object/object-value-calc.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

TEST_CASE("flag_cost values the blessing of either gods the same")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    auto &items = BaseitemList::get_instance();
    items.resize(2);
    auto &base = items.get_baseitem(1);
    base.name = "Test";
    base.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = base.bi_key;
    CHECK(flag_cost(&item, 0) == 0);

    SUBCASE("the blessing of the gods of good")
    {
        item.art_flags.set(TR_BLESSED);
        CHECK(flag_cost(&item, 0) == 750);
    }

    SUBCASE("the blessing of the gods of evil")
    {
        item.art_flags.set(TR_BLESSED_EVIL);
        CHECK(flag_cost(&item, 0) == 750);
    }

    SUBCASE("both blessings are valued only once")
    {
        item.art_flags.set({ TR_BLESSED, TR_BLESSED_EVIL });
        CHECK(flag_cost(&item, 0) == 750);
    }

    SUBCASE("the blessing of the base item is not counted")
    {
        // ベースアイテムが持つ特性の価値は、ベースアイテムの価格に含まれる
        base.flags.set(TR_BLESSED_EVIL);
        CHECK(flag_cost(&item, 0) == 0);
    }
}
