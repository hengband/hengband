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

TEST_CASE("ItemEntity can_be_sensed accepts only the kinds that pseudo-identification handles")
{
    // 定期の擬似鑑定が扱う、武器・防具・矢弾・カード・装飾品の種類
    for (const auto tval : { ItemKindType::SHOT, ItemKindType::ARROW, ItemKindType::BOLT, ItemKindType::BOW, ItemKindType::DIGGING, ItemKindType::HAFTED,
             ItemKindType::POLEARM, ItemKindType::SWORD, ItemKindType::BOOTS, ItemKindType::GLOVES, ItemKindType::HELM, ItemKindType::CROWN,
             ItemKindType::SHIELD, ItemKindType::CLOAK, ItemKindType::SOFT_ARMOR, ItemKindType::HARD_ARMOR, ItemKindType::DRAG_ARMOR, ItemKindType::CARD,
             ItemKindType::RING, ItemKindType::AMULET, ItemKindType::LITE, ItemKindType::FIGURINE }) {
        ItemEntity item;
        item.bi_key = BaseitemKey(tval, 0);
        CAPTURE(static_cast<int>(tval));
        CHECK(item.can_be_sensed());
    }

    for (const auto tval : { ItemKindType::POTION, ItemKindType::SCROLL, ItemKindType::FOOD, ItemKindType::WAND, ItemKindType::STAFF, ItemKindType::ROD, ItemKindType::FLASK }) {
        ItemEntity item;
        item.bi_key = BaseitemKey(tval, 0);
        CAPTURE(static_cast<int>(tval));
        CHECK_FALSE(item.can_be_sensed());
    }
}

TEST_CASE("ItemEntity sense sets the feeling and the sensed flag")
{
    ItemEntity item;
    item.sense(FEEL_EXCELLENT);

    CHECK(item.feeling == FEEL_EXCELLENT);
    CHECK(item.has_identification_flag(IdentificationFlag::SENSE));
}

TEST_CASE("ItemEntity remove_curses clears the curses, the feeling and the sensed flag")
{
    ItemEntity item;
    item.curse_flags.set({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE });
    item.sense(FEEL_CURSED);
    item.remove_curses();

    CHECK_FALSE(item.is_cursed());
    CHECK(item.feeling == FEEL_NONE);
    CHECK_FALSE(item.has_identification_flag(IdentificationFlag::SENSE));
}

TEST_CASE("ItemEntity remove_curses keeps an identified item identified")
{
    ItemEntity item;
    item.curse_flags.set(CurseTraitType::CURSED);
    item.mark_as_known();
    item.remove_curses();

    CHECK_FALSE(item.is_cursed());
    CHECK(item.has_identification_flag(IdentificationFlag::KNOWN));
}

TEST_CASE("ItemEntity add_curses turns the feeling of a sensed item into a cursed one")
{
    ItemEntity item;
    item.sense(FEEL_EXCELLENT);

    CHECK(item.add_curses({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE }));
    CHECK(item.curse_flags.has_all_of({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE }));
    CHECK(item.feeling == FEEL_WORTHLESS);
    CHECK(item.has_identification_flag(IdentificationFlag::SENSE));
}

TEST_CASE("ItemEntity add_curses leaves an unsensed item without a feeling")
{
    ItemEntity item;

    CHECK(item.add_curses({ CurseTraitType::CURSED }));
    CHECK(item.feeling == FEEL_NONE);
    CHECK_FALSE(item.has_identification_flag(IdentificationFlag::SENSE));
}

TEST_CASE("ItemEntity add_curses keeps the feeling when no curse is added")
{
    ItemEntity item;
    item.curse_flags.set(CurseTraitType::CURSED);
    item.sense(FEEL_CURSED);

    CHECK_FALSE(item.add_curses({ CurseTraitType::CURSED }));
    CHECK(item.feeling == FEEL_CURSED);
}

TEST_CASE("ItemEntity add_curses adds the curses when only some of them are new")
{
    ItemEntity item;
    item.curse_flags.set(CurseTraitType::CURSED);
    item.sense(FEEL_AVERAGE);

    CHECK(item.add_curses({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE }));
    CHECK(item.curse_flags.has(CurseTraitType::HEAVY_CURSE));
    CHECK(item.feeling == FEEL_CURSED);
}

TEST_CASE("ItemEntity add_curses keeps a sensed feeling that already shows the curse")
{
    for (const auto feel : { FEEL_TERRIBLE, FEEL_WORTHLESS, FEEL_CURSED }) {
        ItemEntity item;
        item.curse_flags.set(CurseTraitType::CURSED);
        item.sense(feel);
        CAPTURE(static_cast<int>(feel));

        CHECK(item.add_curses({ CurseTraitType::HEAVY_CURSE }));
        CHECK(item.feeling == feel);
    }
}

TEST_CASE("ItemEntity match_feeling_to_curses keeps only a curse feeling on an unsensed item")
{
    ItemEntity cursed_feeling_item;
    cursed_feeling_item.feeling = FEEL_CURSED;
    cursed_feeling_item.match_feeling_to_curses();
    CHECK(cursed_feeling_item.feeling == FEEL_CURSED);

    ItemEntity average_feeling_item;
    average_feeling_item.feeling = FEEL_AVERAGE;
    average_feeling_item.match_feeling_to_curses();
    CHECK(average_feeling_item.feeling == FEEL_NONE);
}

TEST_CASE("ItemEntity match_feeling_to_curses leaves an identified item without a feeling")
{
    ItemEntity item;
    item.mark_as_known();
    item.set_identification_flag(IdentificationFlag::SENSE); // 呪われた品を装備すると鑑定済みでも付く
    item.match_feeling_to_curses();

    CHECK(item.feeling == FEEL_NONE);
}
