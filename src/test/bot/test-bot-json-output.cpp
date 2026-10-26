/*!
 * @brief 制御サーバ・JSONL の出力のテスト
 */

#include "artifact/fixed-art-types.h"
#include "bot/bot-json-output.h"
#include "locale/localized-string.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/baseitem/baseitem-service.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "test/scoped-rng.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/monrace-list-test-access.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <range/v3/algorithm.hpp>
#include <vector>

TEST_CASE("find_bot_knowledge_category maps knowledge menu keys to categories")
{
    CHECK(find_bot_knowledge_category('1') == BotKnowledgeCategory::ARTIFACTS_KNOWN);
    CHECK(find_bot_knowledge_category('2') == BotKnowledgeCategory::ARTIFACTS_IDENTIFIED);
    CHECK(find_bot_knowledge_category('3') == BotKnowledgeCategory::OBJECTS_KNOWN);
    CHECK(find_bot_knowledge_category('4') == BotKnowledgeCategory::UNIQUES_ALIVE);
    CHECK(find_bot_knowledge_category('5') == BotKnowledgeCategory::UNIQUES_DEAD);
    CHECK(find_bot_knowledge_category('6') == BotKnowledgeCategory::MONSTERS);
    CHECK(find_bot_knowledge_category('7') == BotKnowledgeCategory::KILL_COUNT);
    CHECK(find_bot_knowledge_category('8') == BotKnowledgeCategory::BOUNTY);
    CHECK(find_bot_knowledge_category('9') == BotKnowledgeCategory::HOME);
    CHECK(find_bot_knowledge_category('0') == BotKnowledgeCategory::EQUIP_RESISTANCES);
    CHECK(find_bot_knowledge_category('a') == BotKnowledgeCategory::FEATURES);
    CHECK(find_bot_knowledge_category('b') == BotKnowledgeCategory::SELF_INFO);
    CHECK(find_bot_knowledge_category('c') == BotKnowledgeCategory::MUTATIONS);
    CHECK(find_bot_knowledge_category('d') == BotKnowledgeCategory::WEAPON_EXP);
    CHECK(find_bot_knowledge_category('e') == BotKnowledgeCategory::SPELL_EXP);
    CHECK(find_bot_knowledge_category('f') == BotKnowledgeCategory::SKILL_EXP);
    CHECK(find_bot_knowledge_category('g') == BotKnowledgeCategory::VIRTUES);
    CHECK(find_bot_knowledge_category('h') == BotKnowledgeCategory::DUNGEONS);
    CHECK(find_bot_knowledge_category('i') == BotKnowledgeCategory::QUESTS);
    CHECK(find_bot_knowledge_category('j') == BotKnowledgeCategory::PETS);
    CHECK(find_bot_knowledge_category('k') == BotKnowledgeCategory::AUTOPICK);
}

TEST_CASE("find_bot_knowledge_category returns nullopt for keys without a list")
{
    CHECK_FALSE(find_bot_knowledge_category(' ').has_value());
    CHECK_FALSE(find_bot_knowledge_category('l').has_value());
    CHECK_FALSE(find_bot_knowledge_category('A').has_value());
}

namespace {
constexpr short POTION_A_ID = 1;
constexpr short POTION_B_ID = 2;
constexpr short SWORD_ID = 3;

/*!
 * @brief 見た目を入れ替えた薬 2 種と、見た目の無い武器 1 種のベースアイテムを用意する
 * @details 薬 A は薬 B の見た目、薬 B は薬 A の見た目に割り当てる (ゲーム開始時の並べ替えに当たる)。
 */
class ScopedIdentityBaseitems {
public:
    ScopedIdentityBaseitems()
        : saved_items(BaseitemList::get_instance())
        , saved_records(BaseitemRecords::get_instance())
    {
        auto &items = BaseitemList::get_instance();
        items.resize(4);
        items.get_baseitem(POTION_A_ID).bi_key = BaseitemKey(ItemKindType::POTION, 10);
        items.get_baseitem(POTION_B_ID).bi_key = BaseitemKey(ItemKindType::POTION, 20);
        items.get_baseitem(SWORD_ID).bi_key = BaseitemKey(ItemKindType::SWORD, 5);
        auto &records = BaseitemRecords::get_instance();
        records.initialize(4);
        records.get_record(POTION_A_ID).set_appearance_id(POTION_B_ID);
        records.get_record(POTION_B_ID).set_appearance_id(POTION_A_ID);
        records.get_record(SWORD_ID).mark_awareness(true);
    }

    ScopedIdentityBaseitems(const ScopedIdentityBaseitems &) = delete;
    ScopedIdentityBaseitems &operator=(const ScopedIdentityBaseitems &) = delete;
    ScopedIdentityBaseitems(ScopedIdentityBaseitems &&) = delete;
    ScopedIdentityBaseitems &operator=(ScopedIdentityBaseitems &&) = delete;

private:
    test::ScopedVectorWrapper<BaseitemList> saved_items;
    test::ScopedVectorWrapper<BaseitemRecords> saved_records;
};
}

TEST_CASE("make_bot_item_identity_json shows only the flavor of an unaware item")
{
    const ScopedIdentityBaseitems baseitems;
    const ItemEntity item(POTION_A_ID);
    const auto ids = make_bot_item_identity_json(item, false);
    // 名前に出る見た目 (薬 B の flavor_name) の行を指し、中身 (薬 A) の ID や sval は出さない
    CHECK(ids == nlohmann::json{ { "flavor_id", POTION_B_ID } });
}

TEST_CASE("make_bot_item_identity_json follows the flavor shuffle except for a fixed flavor")
{
    test::ScopedVectorWrapper<BaseitemList> saved_items{ BaseitemList::get_instance() };
    test::ScopedVectorWrapper<BaseitemRecords> saved_records{ BaseitemRecords::get_instance() };
    const auto restore_rng = test::scoped_rng();
    constexpr short FIXED_ID = 9;
    auto &items = BaseitemList::get_instance();
    items.resize(FIXED_ID + 1);
    auto &records = BaseitemRecords::get_instance();
    records.initialize(FIXED_ID + 1);
    for (short bi_id = 1; bi_id <= FIXED_ID; ++bi_id) {
        auto &baseitem = items.get_baseitem(bi_id);
        baseitem.name = "Test";
        baseitem.bi_key = BaseitemKey(ItemKindType::POTION, bi_id);
        records.get_record(bi_id).set_appearance_id(bi_id);
    }
    items.get_baseitem(FIXED_ID).flags.set(TR_FIXED_FLAVOR);
    BaseitemService::shuffle_flavors();

    // 並べ替えた見た目は同じ tval の並べ替える品の見た目のどれかで、FIXED_FLAVOR の品は自分の行の見た目のまま
    std::vector<short> flavor_ids;
    for (short bi_id = 1; bi_id < FIXED_ID; ++bi_id) {
        const auto ids = make_bot_item_identity_json(ItemEntity(bi_id), false);
        REQUIRE(ids.contains("flavor_id"));
        flavor_ids.push_back(ids["flavor_id"].get<short>());
    }
    ranges::sort(flavor_ids);
    CHECK(flavor_ids == std::vector<short>{ 1, 2, 3, 4, 5, 6, 7, 8 });
    CHECK(make_bot_item_identity_json(ItemEntity(FIXED_ID), false) == nlohmann::json{ { "flavor_id", FIXED_ID } });
}

TEST_CASE("make_bot_item_identity_json shows nothing for an unaware item without a flavor")
{
    const ScopedIdentityBaseitems baseitems;
    BaseitemRecords::get_instance().get_record(SWORD_ID).mark_awareness(false);
    const ItemEntity item(SWORD_ID);
    CHECK(make_bot_item_identity_json(item, false) == nlohmann::json::object());
}

TEST_CASE("make_bot_item_identity_json shows the sval instead of the flavor once aware")
{
    const ScopedIdentityBaseitems baseitems;
    BaseitemRecords::get_instance().get_record(POTION_A_ID).mark_awareness(true);
    const ItemEntity item(POTION_A_ID);
    CHECK(make_bot_item_identity_json(item, false) == nlohmann::json{ { "sval", 10 } });
}

TEST_CASE("make_bot_item_identity_json hides the ego and the artifact until known")
{
    const ScopedIdentityBaseitems baseitems;
    ItemEntity ego(SWORD_ID);
    ego.ego_idx = EgoType::HA;
    ItemEntity artifact(SWORD_ID);
    artifact.fa_id = FixedArtifactId::JUDGE;

    SUBCASE("unidentified")
    {
        CHECK(make_bot_item_identity_json(ego, false) == nlohmann::json{ { "sval", 5 } });
        CHECK(make_bot_item_identity_json(artifact, false) == nlohmann::json{ { "sval", 5 } });
    }

    SUBCASE("identified")
    {
        ego.mark_as_known();
        artifact.mark_as_known();
        CHECK(make_bot_item_identity_json(ego, false) == nlohmann::json{ { "sval", 5 }, { "ego_id", enum2i(EgoType::HA) } });
        CHECK(make_bot_item_identity_json(artifact, false) == nlohmann::json{ { "sval", 5 }, { "artifact_id", enum2i(FixedArtifactId::JUDGE) } });
    }
}

TEST_CASE("make_bot_item_identity_json omits the artifact id of a random artifact")
{
    const ScopedIdentityBaseitems baseitems;
    ItemEntity item(SWORD_ID);
    item.randart_name = "of Test";
    item.mark_as_known();
    CHECK(make_bot_item_identity_json(item, false) == nlohmann::json{ { "sval", 5 } });
}

TEST_CASE("make_bot_item_identity_json treats store stock as identified")
{
    const ScopedIdentityBaseitems baseitems;
    // 店の商品は、プレイヤーが未判明の種類でも中身を表示し、見た目を出さない
    const ItemEntity potion(POTION_A_ID);
    CHECK(make_bot_item_identity_json(potion, true) == nlohmann::json{ { "sval", 10 } });
    ItemEntity ego(SWORD_ID);
    ego.ego_idx = EgoType::HA;
    CHECK(make_bot_item_identity_json(ego, true) == nlohmann::json{ { "sval", 5 }, { "ego_id", enum2i(EgoType::HA) } });
}

TEST_CASE("make_bot_found_item_identity_json hides the ids while hallucinating")
{
    const ScopedIdentityBaseitems baseitems;
    ItemEntity item(SWORD_ID);
    item.ego_idx = EgoType::HA;
    item.mark_as_known();
    CHECK(make_bot_found_item_identity_json(item, true).is_null());
    CHECK(make_bot_found_item_identity_json(item, false) == make_bot_item_identity_json(item, false));
    const ItemEntity potion(POTION_A_ID);
    CHECK(make_bot_found_item_identity_json(potion, true).is_null());
    CHECK(make_bot_found_item_identity_json(potion, false) == nlohmann::json{ { "flavor_id", POTION_B_ID } });
}

TEST_CASE("make_bot_pet_row_json hides the race while hallucinating")
{
    test::MonraceListTestAccess monraces;
    auto &monrace = MonraceList::get_instance().emplace(static_cast<MonraceId>(1));
    monrace.name = LocalizedString("Test pet", "Test pet");
    MonsterEntity monster;
    monster.r_idx = static_cast<MonraceId>(1);
    CHECK(make_bot_pet_row_json(5, monster, false) == nlohmann::json{ { "index", 5 }, { "race_id", 1 }, { "name", "Test pet" } });
    CHECK(make_bot_pet_row_json(5, monster, true) == nlohmann::json{ { "index", 5 }, { "hallucinated", true } });
}
