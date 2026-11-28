/*!
 * @brief 武器の修復 (market/building-craft-fix.cpp) のテスト
 *
 * 修復材料から修復対象へ移植できる特性の候補の集め方を確かめる。
 */

#include "market/building-craft-fix.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "test/scoped-vector-wrapper.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <vector>

namespace {

/*!
 * @brief 特性を持たない剣のベースアイテムを 1 種類だけ用意し、そのアイテムを返す
 * @details 呼び出し側は、先に ScopedVectorWrapper<BaseitemList> でベースアイテムの一覧を退避しておく。
 */
ItemEntity make_plain_sword()
{
    auto &baseitems = BaseitemList::get_instance();
    baseitems.resize(2);
    auto &baseitem = baseitems.get_baseitem(1);
    baseitem.name = "Test";
    baseitem.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = baseitem.bi_key;
    return item;
}

bool contains(const std::vector<tr_type> &candidates, tr_type flag)
{
    return std::find(candidates.begin(), candidates.end(), flag) != candidates.end();
}

}

TEST_CASE("collect_transplantable_abilities picks abilities only the material has")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto to_item = make_plain_sword();
    auto from_item = make_plain_sword();
    to_item.art_flags.set(TR_SLAY_EVIL);
    from_item.art_flags.set({ TR_SLAY_EVIL, TR_VORPAL });

    const auto candidates = collect_transplantable_abilities(to_item, from_item);
    CHECK(candidates == std::vector<tr_type>{ TR_VORPAL });
}

TEST_CASE("collect_transplantable_abilities skips the abilities that are never transplanted")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    const auto to_item = make_plain_sword();
    auto from_item = make_plain_sword();
    from_item.art_flags.set({ TR_IGNORE_FIRE, TR_ACTIVATE, TR_RIDING, TR_THROW, TR_SHOW_MODS });

    CHECK(collect_transplantable_abilities(to_item, from_item).empty());
}

TEST_CASE("collect_transplantable_abilities needs a positive pval for the abilities tied to pval")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    const auto to_item = make_plain_sword();
    auto from_item = make_plain_sword();
    from_item.art_flags.set({ TR_STR, TR_VORPAL });

    from_item.pval = 0;
    CHECK(collect_transplantable_abilities(to_item, from_item) == std::vector<tr_type>{ TR_VORPAL });

    from_item.pval = 1;
    const auto candidates = collect_transplantable_abilities(to_item, from_item);
    CHECK(candidates.size() == 2);
    CHECK(contains(candidates, TR_STR));
    CHECK(contains(candidates, TR_VORPAL));
}

TEST_CASE("collect_transplantable_abilities transplants a blessing unless the target has the other one")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto to_item = make_plain_sword();
    auto from_item = make_plain_sword();

    SUBCASE("the blessing of the gods of good goes to a weapon without a blessing")
    {
        from_item.art_flags.set(TR_BLESSED);
        CHECK(collect_transplantable_abilities(to_item, from_item) == std::vector<tr_type>{ TR_BLESSED });
    }

    SUBCASE("the blessing of the gods of evil goes to a weapon without a blessing")
    {
        from_item.art_flags.set(TR_BLESSED_EVIL);
        CHECK(collect_transplantable_abilities(to_item, from_item) == std::vector<tr_type>{ TR_BLESSED_EVIL });
    }

    SUBCASE("the blessing of the gods of evil does not go to a weapon blessed by the gods of good")
    {
        to_item.art_flags.set(TR_BLESSED);
        from_item.art_flags.set({ TR_BLESSED_EVIL, TR_VORPAL });
        CHECK(collect_transplantable_abilities(to_item, from_item) == std::vector<tr_type>{ TR_VORPAL });
    }

    SUBCASE("the blessing of the gods of good does not go to a weapon blessed by the gods of evil")
    {
        to_item.art_flags.set(TR_BLESSED_EVIL);
        from_item.art_flags.set({ TR_BLESSED, TR_VORPAL });
        CHECK(collect_transplantable_abilities(to_item, from_item) == std::vector<tr_type>{ TR_VORPAL });
    }

    SUBCASE("the same blessing is not a candidate again")
    {
        to_item.art_flags.set(TR_BLESSED_EVIL);
        from_item.art_flags.set(TR_BLESSED_EVIL);
        CHECK(collect_transplantable_abilities(to_item, from_item).empty());
    }
}
