#include "artifact/fixed-art-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/item/item-entity.h"
#include "test/scoped-ego-list.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include <doctest/doctest.h>
#include <initializer_list>
#include <limits>
#include <utility>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("ItemEntity calc_price discounts large prices without multiplication overflow")
{
    test::ScopedEgoList saved_egos;
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
    item.set_identification_flag(IdentificationFlag::SENSE); // 古いセーブに残りうる状態 (以前は呪われた品を装備すると鑑定済みでも付いた)
    item.match_feeling_to_curses();

    CHECK(item.feeling == FEEL_NONE);
}

TEST_CASE("ItemEntity find_curse_feeling gives a cursed feeling to a cursed item that can be sensed")
{
    ItemEntity item;
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
    CHECK_FALSE(item.find_curse_feeling().has_value());

    item.curse_flags.set(CurseTraitType::CURSED);
    CHECK(item.find_curse_feeling() == FEEL_CURSED);

    item.sense(FEEL_SPECIAL);
    CHECK(item.find_curse_feeling() == FEEL_TERRIBLE);

    item.sense(FEEL_EXCELLENT);
    CHECK(item.find_curse_feeling() == FEEL_WORTHLESS);

    item.sense(FEEL_NONE); // 古いセーブに残りうる状態 (以前は呪われた品を装備すると感触の無いまま付いた)
    CHECK(item.find_curse_feeling() == FEEL_CURSED);

    item.sense(FEEL_BROKEN); // 壊れているは呪いを表さない
    CHECK(item.find_curse_feeling() == FEEL_CURSED);
}

TEST_CASE("ItemEntity find_curse_feeling uses only a curse feeling of an unsensed item")
{
    // 修正前の呪い感知は SENSE を付けずに感触を付けていた
    for (const auto feel : { FEEL_TERRIBLE, FEEL_WORTHLESS, FEEL_CURSED }) {
        ItemEntity item;
        item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
        item.curse_flags.set(CurseTraitType::CURSED);
        item.feeling = feel;
        CAPTURE(static_cast<int>(feel));
        CHECK(item.find_curse_feeling() == feel);
    }

    ItemEntity item;
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
    item.curse_flags.set(CurseTraitType::CURSED);
    item.feeling = FEEL_EXCELLENT;
    CHECK(item.find_curse_feeling() == FEEL_CURSED);
}

TEST_CASE("ItemEntity find_curse_feeling skips items whose feeling cannot change")
{
    for (const auto feel : { FEEL_TERRIBLE, FEEL_WORTHLESS, FEEL_CURSED }) {
        ItemEntity sensed_item;
        sensed_item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
        sensed_item.curse_flags.set(CurseTraitType::CURSED);
        sensed_item.sense(feel);
        CAPTURE(static_cast<int>(feel));
        CHECK_FALSE(sensed_item.find_curse_feeling().has_value());
    }

    ItemEntity uncursed_item;
    uncursed_item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
    uncursed_item.sense(FEEL_AVERAGE);
    CHECK_FALSE(uncursed_item.find_curse_feeling().has_value());

    ItemEntity known_item;
    known_item.bi_key = BaseitemKey(ItemKindType::SWORD, 0);
    known_item.curse_flags.set(CurseTraitType::CURSED);
    known_item.mark_as_known();
    CHECK_FALSE(known_item.find_curse_feeling().has_value());

    ItemEntity potion;
    potion.bi_key = BaseitemKey(ItemKindType::POTION, 0);
    potion.curse_flags.set(CurseTraitType::CURSED);
    CHECK_FALSE(potion.find_curse_feeling().has_value());
}

TEST_CASE("ItemEntity is_blessed finds the blessing on the base item, the ego and the item itself")
{
    test::ScopedEgoList saved_egos;
    auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    auto &items = BaseitemList::get_instance();
    items.resize(2);
    auto &base = items.get_baseitem(1);
    base.name = "Test";
    base.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = base.bi_key;

    SUBCASE("an item without the flag is not blessed")
    {
        // 祝福と関係のない特性は祝福として数えない
        base.flags.set(TR_VORPAL);
        item.art_flags.set(TR_SLAY_EVIL);
        CHECK_FALSE(item.is_blessed());
    }

    SUBCASE("the blessing of the base item counts")
    {
        base.flags.set(TR_BLESSED);
        CHECK(item.is_blessed());
    }

    SUBCASE("the blessing of the ego counts only for an item of that ego")
    {
        ego.flags.set(TR_BLESSED);
        CHECK_FALSE(item.is_blessed());

        item.ego_idx = EgoType::A_MORGUL;
        CHECK(item.is_blessed());
    }

    SUBCASE("the blessing of the fixed artifact counts")
    {
        // 固定アーティファクトの特性は、ベースアイテムの特性を置き換える
        test::ArtifactListTestAccess saved_artifacts;
        ArtifactDefinition blessed_artifact;
        blessed_artifact.flags.set(TR_BLESSED);
        ArtifactList::get_instance().emplace(FixedArtifactId::JUDGE, std::move(blessed_artifact));
        ArtifactList::get_instance().emplace(FixedArtifactId::POWER, ArtifactDefinition{});

        item.fa_id = FixedArtifactId::JUDGE;
        CHECK(item.is_blessed());

        base.flags.set(TR_BLESSED);
        item.fa_id = FixedArtifactId::POWER;
        CHECK_FALSE(item.is_blessed());
    }

    SUBCASE("the blessing given to the item itself counts")
    {
        item.art_flags.set(TR_BLESSED);
        CHECK(item.is_blessed());
    }

    SUBCASE("the blessing counts even on an unidentified item")
    {
        // 鑑定の状態は見ない (get_flags_known() ではなく get_flags() で判定する)
        item.art_flags.set(TR_BLESSED);
        CHECK(item.has_not_identification_flag(IdentificationFlag::KNOWN));
        CHECK(item.has_not_identification_flag(IdentificationFlag::FULL_KNOWN));
        CHECK(item.is_blessed());
    }
}

TEST_CASE("ItemEntity is_unblessed_edged_weapon is true only for a sword or a polearm without the blessing")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    auto &items = BaseitemList::get_instance();
    items.resize(2);
    auto &base = items.get_baseitem(1);
    base.name = "Test";
    ItemEntity item;
    item.bi_id = 1;

    SUBCASE("a sword or a polearm is unsuitable until it is blessed")
    {
        for (const auto tval : { ItemKindType::SWORD, ItemKindType::POLEARM }) {
            CAPTURE(static_cast<int>(tval));
            base.bi_key = BaseitemKey(tval, 1);
            item.bi_key = base.bi_key;
            item.art_flags.clear();
            CHECK(item.is_unblessed_edged_weapon());

            item.art_flags.set(TR_BLESSED);
            CHECK_FALSE(item.is_unblessed_edged_weapon());
        }
    }

    SUBCASE("the blessing of the base item also makes an edged weapon suitable")
    {
        base.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
        base.flags.set(TR_BLESSED);
        item.bi_key = base.bi_key;
        CHECK_FALSE(item.is_unblessed_edged_weapon());
    }

    SUBCASE("an item that is not an edged weapon is never unsuitable")
    {
        // 鈍器、掘削具、射撃武器、矢、盾は、祝福が無くても罰の対象にならない
        for (const auto tval : { ItemKindType::HAFTED, ItemKindType::DIGGING, ItemKindType::BOW, ItemKindType::ARROW, ItemKindType::SHIELD }) {
            CAPTURE(static_cast<int>(tval));
            base.bi_key = BaseitemKey(tval, 1);
            item.bi_key = base.bi_key;
            item.art_flags.clear();
            CHECK_FALSE(item.is_unblessed_edged_weapon());

            item.art_flags.set(TR_BLESSED);
            CHECK_FALSE(item.is_unblessed_edged_weapon());
        }
    }

    SUBCASE("an empty slot is not unsuitable")
    {
        // 何も持っていない手のアイテムは、ベースアイテムを引かずに偽を返す
        const ItemEntity empty_item;
        CHECK_FALSE(empty_item.is_valid());
        CHECK_FALSE(empty_item.is_unblessed_edged_weapon());
    }
}
