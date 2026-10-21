#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/language-switcher.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <string_view>
#include <vector>

namespace {
// 無効なベースアイテムの名前（「異常アイテム」）。2バイト文字のテストデータは16進エスケープで書く。
#if defined(JP) && defined(SJIS)
constexpr std::string_view INVALID_ITEM_NAME = "\x88\xd9\x8f\xed\x83\x41\x83\x43\x83\x65\x83\x80";
#elif defined(JP)
constexpr std::string_view INVALID_ITEM_NAME = "\xb0\xdb\xbe\xef\xa5\xa2\xa5\xa4\xa5\xc6\xa5\xe0";
#endif

/*!
 * @brief アーティファクトの元にするベースアイテム（種別 1・副種別 2）を一時的に用意する
 */
class ScopedArtifactBaseitem {
public:
    ScopedArtifactBaseitem()
    {
        auto &baseitems = BaseitemList::get_instance();
        baseitems.resize(2);
        BaseitemDefinition baseitem;
        baseitem.name = "Test";
        baseitem.bi_key = { ItemKindType::FLAVOR_SKELETON, 2 };
        baseitems.replace_baseitem(1, std::move(baseitem));
    }

    ScopedArtifactBaseitem(const ScopedArtifactBaseitem &) = delete;
    ScopedArtifactBaseitem &operator=(const ScopedArtifactBaseitem &) = delete;
    ScopedArtifactBaseitem(ScopedArtifactBaseitem &&) = delete;
    ScopedArtifactBaseitem &operator=(ScopedArtifactBaseitem &&) = delete;

private:
    test::ScopedVectorWrapper<BaseitemList> items{ BaseitemList::get_instance() };
};

nlohmann::json make_artifact(int id = 1)
{
    return {
        { "id", id },
        { "name", { { "ja", "Test JA" }, { "en", "Test EN" } } },
        { "base_item", { { "type_value", 1 }, { "subtype_value", 2 } } },
        { "level", 3 },
        { "rarity", 4 },
        { "weight", 5 },
        { "cost", 6 },
    };
}
}

TEST_CASE("ArtifactReader validates ID range before narrowing signed and unsigned integers")
{
    const auto invalid_ids = std::vector<nlohmann::json>{
        -1,
        10000,
        32768,
        65537,
        std::numeric_limits<int>::max(),
        std::numeric_limits<nlohmann::json::number_integer_t>::min(),
        std::numeric_limits<nlohmann::json::number_integer_t>::max(),
        nlohmann::json::number_integer_t{ 4294967297LL },
        nlohmann::json::number_integer_t{ -4294967295LL },
        nlohmann::json::number_unsigned_t{ 10000 },
        nlohmann::json::number_unsigned_t{ 4294967297ULL },
        std::numeric_limits<nlohmann::json::number_unsigned_t>::max(),
    };
    for (const auto &id : invalid_ids) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        auto data = make_artifact();
        data["id"] = id;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(ArtifactList::get_instance().empty());
        CHECK(error_idx == -1);
    }
}

TEST_CASE("ArtifactReader rejects rarity values outside its storage type")
{
    for (const auto value : { -1, 256 }) {
        CAPTURE(value);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        auto data = make_artifact();
        data["rarity"] = value;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(ArtifactList::get_instance().empty());
    }
}

TEST_CASE("ArtifactReader preserves missing and non-integer ID errors")
{
    for (const auto &id : { nlohmann::json(), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        auto data = make_artifact();
        data["id"] = id;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
        CHECK(error_idx == -1);
        CHECK(ArtifactList::get_instance().empty());
    }
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto data = make_artifact();
    data.erase("id");
    CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
    const auto non_object = nlohmann::json::array();
    CHECK(test::to_error_code(ArtifactReader(non_object).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(error_idx == -1);
    CHECK(ArtifactList::get_instance().empty());
}

/*!
 * @brief ID は 1 から始まり、欠番なく並んでいなければならない
 * @details 実行時の記録は 1〜件数の ID だけを作るため、0・欠番・重複・逆順を拒否する。
 */
TEST_CASE("ArtifactReader requires IDs to start at 1 and continue without gaps")
{
    for (const auto &id : { nlohmann::json(0), nlohmann::json(2), nlohmann::json(nlohmann::json::number_unsigned_t{ 0 }), nlohmann::json(9999) }) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        auto data = make_artifact();
        data["id"] = id;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
        CHECK(ArtifactList::get_instance().empty());
        CHECK(error_idx == -1);
    }

    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto &artifacts = ArtifactList::get_instance();
    const auto read_id = [](int id) {
        const auto data = make_artifact(id);
        return ArtifactReader(data).read();
    };
    REQUIRE(read_id(1).has_value());
    for (const auto id : { 1, 3, 0 }) {
        CAPTURE(id);
        CHECK(test::to_error_code(read_id(id)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
        CHECK(artifacts.size() == 1);
        CHECK(error_idx == 1);
    }
    REQUIRE(read_id(2).has_value());
    CHECK(artifacts.size() == 2);
    CHECK(error_idx == 2);
}

/*!
 * @brief 直前の ID が 9998 なら、ID の範囲の上限 9999 を受け付ける
 * @details 診断のための ID を直接進めて、範囲の検査だけを確かめる（一覧に 9999 だけがある状態は実行時には起きない）。
 */
TEST_CASE("ArtifactReader accepts the largest schema ID after its predecessor")
{
    for (const auto &id : { nlohmann::json(9999), nlohmann::json(nlohmann::json::number_unsigned_t{ 9999 }) }) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        error_idx = 9998;
        auto data = make_artifact();
        data["id"] = id;
        REQUIRE(ArtifactReader(data).read().has_value());
        auto &artifacts = ArtifactList::get_instance();
        REQUIRE(artifacts.size() == 1);
        CHECK(enum2i(artifacts.begin()->first) == 9999);
        CHECK(error_idx == 9999);
    }
}

/*!
 * @brief 元のベースアイテムが有効なベースアイテムに無ければ、ほかの項目を読み終えた後で拒否する
 * @details 生成・鑑定では元のベースアイテムを種別・副種別から引き、無ければ例外で落ちる。
 */
TEST_CASE("ArtifactReader rejects a base item which is not a valid baseitem")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto &baseitems = BaseitemList::get_instance();
    auto data = make_artifact();
    SUBCASE("missing subtype")
    {
        data["base_item"]["subtype_value"] = 3;
    }
    SUBCASE("missing type")
    {
        data["base_item"]["type_value"] = 2;
    }
    SUBCASE("invalid baseitem")
    {
        BaseitemDefinition invalid;
        invalid.name = _(INVALID_ITEM_NAME, "Invalid item");
        invalid.bi_key = { ItemKindType::FLAVOR_SKELETON, 2 };
        baseitems.replace_baseitem(1, std::move(invalid));
    }
    SUBCASE("unnamed baseitem")
    {
        BaseitemDefinition unnamed;
        unnamed.bi_key = { ItemKindType::FLAVOR_SKELETON, 2 };
        baseitems.replace_baseitem(1, std::move(unnamed));
    }
    CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::INVALID_VALUE);
    CHECK(ArtifactList::get_instance().empty());
    // ID の確認は済んでいるため、診断のための ID は進む。
    CHECK(error_idx == 1);
}

/*!
 * @brief ダイスの個数と面数は、セーブの1バイトに収まる 0〜255 に限る
 */
TEST_CASE("ArtifactReader limits base dice to the save format range")
{
    for (const auto *dice : { "256d1", "1d256", "-1d2" }) {
        CAPTURE(dice);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        ScopedArtifactBaseitem baseitem;
        auto data = make_artifact();
        data["base_dice"] = dice;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(ArtifactList::get_instance().empty());
    }
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto data = make_artifact();
    data["base_dice"] = "255d255";
    REQUIRE(ArtifactReader(data).read().has_value());
    CHECK(ArtifactList::get_instance().get_artifact(i2enum<FixedArtifactId>(1)).damage_dice == Dice(255, 255));
}

TEST_CASE("ArtifactReader publishes complete records with localized text and optional fields")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto data = make_artifact();
    data["parameter_value"] = -7;
    data["base_ac"] = 8;
    data["base_dice"] = "2d9";
    data["hit_bonus"] = -10;
    data["damage_bonus"] = 11;
    data["ac_bonus"] = 12;
    data["activate"] = "BA_FIRE_4";
    data["flags"] = { "STR", "INSTA_ART" };
    data["flavor"] = { { "ja", "Flavor JA" }, { "en", "Flavor EN" } };
    REQUIRE(ArtifactReader(data).read().has_value());
    const auto &artifact = ArtifactList::get_instance().get_artifact(i2enum<FixedArtifactId>(1));
#ifdef JP
    CHECK(artifact.name == "Test JA");
    CHECK(artifact.text == "Flavor JA");
#else
    CHECK(artifact.name == "Test EN");
    CHECK(artifact.text == "Flavor EN");
#endif
    CHECK(artifact.bi_key == BaseitemKey(ItemKindType::FLAVOR_SKELETON, 2));
    CHECK(artifact.level == 3);
    CHECK(artifact.rarity == 4);
    CHECK(artifact.weight == 5);
    CHECK(artifact.cost == 6);
    CHECK(artifact.pval == -7);
    CHECK(artifact.ac == 8);
    CHECK(artifact.damage_dice == Dice(2, 9));
    CHECK(artifact.to_h == -10);
    CHECK(artifact.to_d == 11);
    CHECK(artifact.to_a == 12);
    CHECK(artifact.act_idx == RandomArtActType::BA_FIRE_4);
    CHECK(artifact.flags.has(TR_ACTIVATE));
    CHECK(artifact.flags.has(TR_STR));
    CHECK(artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART));
    for (const auto flag : { TR_IGNORE_ACID, TR_IGNORE_ELEC, TR_IGNORE_FIRE, TR_IGNORE_COLD }) {
        CHECK(artifact.flags.has(flag));
    }
    CHECK(error_idx == 1);
}

TEST_CASE("ArtifactReader preserves defaults when optional fields are omitted")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    const auto data = make_artifact();
    REQUIRE(ArtifactReader(data).read().has_value());
    const auto &artifact = ArtifactList::get_instance().get_artifact(i2enum<FixedArtifactId>(1));
    CHECK(artifact.text.empty());
    CHECK(artifact.pval == 0);
    CHECK(artifact.ac == 0);
    CHECK(artifact.to_h == 0);
    CHECK(artifact.to_d == 0);
    CHECK(artifact.to_a == 0);
    CHECK(artifact.damage_dice == Dice());
    CHECK(artifact.act_idx == RandomArtActType::NONE);
    CHECK_FALSE(artifact.flags.has(TR_ACTIVATE));
}

TEST_CASE("ArtifactReader preserves existing definitions after early and late parse errors")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    ScopedArtifactBaseitem baseitem;
    auto &artifacts = ArtifactList::get_instance();
    const auto original_data = make_artifact();
    REQUIRE(ArtifactReader(original_data).read().has_value());
    const auto *original = std::addressof(artifacts.get_artifact(i2enum<FixedArtifactId>(1)));
    auto data = make_artifact(2);
    data["activate"] = "BA_FIRE_4";
    data["flags"] = { "STR" };
    auto expected_error = ParseErrorType::INVALID_TYPE;
    SUBCASE("early name error")
    {
        data["name"] = 42;
    }
    SUBCASE("late flag error after a valid flag and activation")
    {
        data["flags"] = { "STR", "UNKNOWN_FLAG" };
        expected_error = ParseErrorType::INVALID_FLAG;
    }
    SUBCASE("last flavor error after all other fields")
    {
        data["flavor"] = 42;
    }
    for (const auto id : { 1, 2 }) {
        CAPTURE(id);
        error_idx = id - 1;
        data["id"] = id;
        CHECK(test::to_error_code(ArtifactReader(data).read()) == expected_error);
        REQUIRE(artifacts.size() == 1);
        CHECK(std::addressof(artifacts.get_artifact(i2enum<FixedArtifactId>(1))) == original);
#ifdef JP
        CHECK(original->name == "Test JA");
#else
        CHECK(original->name == "Test EN");
#endif
        CHECK(original->cost == 6);
        CHECK(original->act_idx == RandomArtActType::NONE);
        CHECK_FALSE(original->flags.has(TR_STR));
        CHECK_FALSE(original->flags.has(TR_ACTIVATE));
        CHECK(original->text.empty());
        CHECK(error_idx == id);
    }
}
