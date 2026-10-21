#include "artifact/random-art-effects.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/baseitem-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/item-reader-limits.h"
#include "info-reader/parse-error-types.h"
#include "locale/language-switcher.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "util/dice.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
class BaseitemStateGuard {
public:
    BaseitemStateGuard() = default;
    BaseitemStateGuard(const BaseitemStateGuard &) = delete;
    BaseitemStateGuard &operator=(const BaseitemStateGuard &) = delete;

private:
    test::ScopedVectorWrapper<BaseitemList> items{ BaseitemList::get_instance() };
    test::ScopedReaderState reader_state;
};

nlohmann::json make_baseitem(int id = 1)
{
    return {
        { "id", id },
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "symbol", { { "character", "!" }, { "color", "White" } } },
        { "itemkind", { { "type_value", 1 }, { "subtype_value", 1 } } },
        { "level", 2 },
        { "weight", 3 },
        { "cost", 4 },
    };
}
}

TEST_CASE("BaseitemReader does not grow the list after a parse error")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    auto data = make_baseitem(7);
    SUBCASE("early name error")
    {
        data["name"] = 42;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_TYPE);
    }
    SUBCASE("late activation error")
    {
        data["activate"] = "UNKNOWN_ACTIVATION";
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_FLAG);
    }
    SUBCASE("late flag error after a valid flag")
    {
        data["flags"] = { "STR", "UNKNOWN_FLAG" };
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_FLAG);
    }
    CHECK(items.empty());
    CHECK(error_idx == 7);
}

TEST_CASE("BaseitemReader preserves helper error precedence without replacing existing records")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const auto original = make_baseitem();
    REQUIRE(BaseitemReader(original).read().has_value());
    auto &existing = items.get_baseitem(1);
    existing.name = "Existing";
    existing.cost = 91;
    existing.pval = 23;
    existing.alloc_tables[0] = { 12, 13 };
    existing.act_idx = RandomArtActType::BA_FIRE_4;
    existing.flags.set(TR_ACTIVATE);
    existing.flags.set(TR_STR);
    const auto previous_ids = items.collect_valid_bi_ids();
    auto data = make_baseitem();
    data["name"] = { { "ja", "Changed" }, { "en", "Changed" } };
    auto expected = ParseErrorType::INVALID_TYPE;
    SUBCASE("symbol before item kind")
    {
        data["symbol"]["character"] = "!!";
        data["itemkind"] = 42;
        expected = ParseErrorType::INVALID_VALUE;
    }
    SUBCASE("item kind before parameter value")
    {
        data["itemkind"].erase("subtype_value");
        data["parameter_value"] = "bad";
        expected = ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    SUBCASE("rod parameter value before level")
    {
        data["itemkind"]["type_value"] = static_cast<int>(ItemKindType::ROD);
        data["parameter_value"] = 0;
        data["level"] = "bad";
        expected = ParseErrorType::INVALID_PVAL;
    }
    SUBCASE("allocation entries before activation")
    {
        data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } }, { { "depth", 6 }, { "rarity", "bad" } } };
        data["activate"] = "UNKNOWN_ACTIVATION";
    }
    SUBCASE("activation before flags")
    {
        data["activate"] = "UNKNOWN_ACTIVATION";
        data["flags"] = 42;
        expected = ParseErrorType::INVALID_FLAG;
    }
    SUBCASE("invalid flag element after valid activation and flag")
    {
        data["activate"] = "LIGHT";
        data["flags"] = { "DEX", 42 };
    }
    SUBCASE("flags after allocations and activation")
    {
        data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } } };
        data["activate"] = "LIGHT";
        data["flags"] = { "STR", "DEX", "UNKNOWN_FLAG" };
        expected = ParseErrorType::INVALID_FLAG;
    }
    error_idx = -1;
    CHECK(test::to_error_code(BaseitemReader(data).read()) == expected);
    CHECK(error_idx == 1);
    REQUIRE(items.size() == 2);
    CHECK(std::addressof(items.get_baseitem(1)) == std::addressof(existing));
    CHECK(existing.name == "Existing");
    CHECK(existing.cost == 91);
    CHECK(existing.pval == 23);
    CHECK(existing.alloc_tables[0].level == 12);
    CHECK(existing.alloc_tables[0].chance == 13);
    CHECK(existing.act_idx == RandomArtActType::BA_FIRE_4);
    CHECK(existing.flags.has(TR_STR));
    CHECK_FALSE(existing.flags.has(TR_DEX));
    CHECK(existing.flags.has(TR_ACTIVATE));
    CHECK(items.collect_valid_bi_ids() == previous_ids);
}

TEST_CASE("BaseitemReader publishes complete records and preserves omitted optional fields")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    auto data = make_baseitem(3);
    data["activate"] = "LIGHT";
    data["flags"] = { "STR" };
    data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } } };
    REQUIRE(BaseitemReader(data).read().has_value());
    REQUIRE(items.size() == 4);
    auto &existing = items.get_baseitem(3);
    CHECK(existing.name == "Test");
    CHECK(existing.cost == 4);
    CHECK(existing.level == 2);
    CHECK(existing.weight == 3);
    CHECK(existing.act_idx == RandomArtActType::LIGHT);
    CHECK(existing.flags.has(TR_STR));
    CHECK(existing.flags.has(TR_ACTIVATE));
    CHECK(existing.alloc_tables[0].level == 4);
    CHECK(existing.alloc_tables[0].chance == 5);
    existing.text = "Flavor";
    existing.flavor_name = "Unknown";
    existing.pval = 6;
    existing.ac = 7;
    existing.to_h = 8;
    existing.to_d = 9;
    existing.to_a = 10;
    existing.damage_dice = Dice(2, 3);
    existing.easy_know = true;
    data = make_baseitem(3);
    data["cost"] = 17;
    error_idx = -1;
    REQUIRE(BaseitemReader(data).read().has_value());
    const auto &updated = items.get_baseitem(3);
    CHECK(updated.cost == 17);
    CHECK(updated.text == "Flavor");
    CHECK(updated.flavor_name == "Unknown");
    CHECK(updated.pval == 6);
    CHECK(updated.ac == 7);
    CHECK(updated.to_h == 8);
    CHECK(updated.to_d == 9);
    CHECK(updated.to_a == 10);
    CHECK(updated.damage_dice == Dice(2, 3));
    CHECK(updated.easy_know);
    CHECK(updated.act_idx == RandomArtActType::LIGHT);
    CHECK(updated.flags.has(TR_STR));
    CHECK(updated.flags.has(TR_ACTIVATE));
    CHECK(updated.alloc_tables[0].level == 4);
    CHECK(updated.alloc_tables[0].chance == 5);
    CHECK(error_idx == 3);
    CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
}

TEST_CASE("BaseitemReader validates full-width integer IDs before narrowing or publishing")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const auto original = make_baseitem();
    REQUIRE(BaseitemReader(original).read().has_value());
    const auto *existing = std::addressof(items.get_baseitem(1));
    const auto invalid_ids = std::vector<nlohmann::json>{
        32768,
        65537,
        std::numeric_limits<int>::max(),
        std::numeric_limits<nlohmann::json::number_integer_t>::max(),
        nlohmann::json::number_integer_t{ 4294967297LL },
        nlohmann::json::number_integer_t{ -4294967295LL },
        std::numeric_limits<nlohmann::json::number_integer_t>::min(),
        -1,
        nlohmann::json::number_unsigned_t{ 32768 },
        nlohmann::json::number_unsigned_t{ 4294967297ULL },
        std::numeric_limits<nlohmann::json::number_unsigned_t>::max(),
    };
    for (const auto &id : invalid_ids) {
        CAPTURE(id);
        auto data = make_baseitem();
        data["id"] = id;
        data["cost"] = 91;
        error_idx = -1;
        const auto is_negative = !id.is_number_unsigned() && id.get<nlohmann::json::number_integer_t>() < 0;
        const auto expected = is_negative ? ParseErrorType::NON_SEQUENTIAL_RECORDS : ParseErrorType::OUT_OF_BOUNDS;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == expected);
        REQUIRE(items.size() == 2);
        CHECK(std::addressof(items.get_baseitem(1)) == existing);
        CHECK(existing->name == "Test");
        CHECK(existing->cost == 4);
        CHECK(error_idx == -1);
    }
}

TEST_CASE("BaseitemReader preserves missing and wrong-type ID errors without publishing")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    for (const auto &id : { nlohmann::json(), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        CAPTURE(id);
        auto data = make_baseitem();
        data["id"] = id;
        const auto expected = id.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == expected);
        CHECK(items.empty());
        CHECK(error_idx == -1);
    }
    auto data = make_baseitem();
    data.erase("id");
    CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::TOO_FEW_ARGUMENTS);
    CHECK(items.empty());
    CHECK(error_idx == -1);
}

TEST_CASE("BaseitemReader accepts signed and unsigned zero IDs")
{
    for (const auto &id : { nlohmann::json(0), nlohmann::json(nlohmann::json::number_unsigned_t{ 0 }) }) {
        CAPTURE(id);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["id"] = id;
        REQUIRE(BaseitemReader(data).read().has_value());
        auto &items = BaseitemList::get_instance();
        REQUIRE(items.size() == 1);
        CHECK(items.get_baseitem(0).name == "Test");
        CHECK(error_idx == 0);
    }
}

TEST_CASE("BaseitemReader preserves the short ID upper bound independently of the schema cap")
{
    const auto ids = std::vector<nlohmann::json>{
        9999,
        10000,
        32767,
        nlohmann::json::number_unsigned_t{ 32767 },
    };
    for (const auto &id : ids) {
        CAPTURE(id);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["id"] = id;
        // Prove the ID passed validation without allocating 32768 definitions.
        data["name"] = 42;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_TYPE);
        CHECK(error_idx == id.get<int>());
        CHECK(BaseitemList::get_instance().empty());
    }
}

TEST_CASE("BaseitemReader publishes accessible ID 32767 with 32768 entries")
{
    BaseitemStateGuard baseitems_guard;
    const auto data = make_baseitem(32767);
    REQUIRE(BaseitemReader(data).read().has_value());

    auto &baseitems = BaseitemList::get_instance();
    REQUIRE(baseitems.size() == 32768);
    CHECK(baseitems.is_valid(32767));
    CHECK_FALSE(baseitems.is_valid(32766));
    CHECK(baseitems.get_baseitem(32767).cost == 4);
    CHECK_THROWS_AS(baseitems.get_baseitem(-1), std::logic_error);
}

TEST_CASE("BaseitemReader preserves ordering precedence and rejects negative indices")
{
    BaseitemStateGuard guard;
    auto data = make_baseitem();
    auto expected = ParseErrorType::NON_SEQUENTIAL_RECORDS;
    auto previous = 1;
    SUBCASE("duplicate")
    {
        data["id"] = 1;
    }
    SUBCASE("descending unsigned zero")
    {
        data["id"] = nlohmann::json::number_unsigned_t{ 0 };
    }
    SUBCASE("out-of-range ID below the previous ID still fails ordering first")
    {
        data["id"] = nlohmann::json::number_unsigned_t{ 32768 };
        previous = 40000;
    }
    SUBCASE("negative ID above an abnormal negative previous ID cannot index the list")
    {
        data["id"] = -1;
        previous = -100;
        expected = ParseErrorType::OUT_OF_BOUNDS;
    }
    error_idx = previous;
    CHECK(test::to_error_code(BaseitemReader(data).read()) == expected);
    CHECK(error_idx == previous);
    CHECK(BaseitemList::get_instance().empty());
}

TEST_CASE("BaseitemReader publishes nested helper values only after successful parsing")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    auto data = make_baseitem(4);
    data["symbol"] = { { "character", "-" }, { "color", "Red" } };
    data["itemkind"] = { { "type_value", static_cast<int>(ItemKindType::ROD) }, { "subtype_value", 2 } };
    data["parameter_value"] = 9;
    data["allocations"] = { { { "depth", 4 }, { "rarity", 5 } }, { { "depth", 6 }, { "rarity", 7 } } };
    data["activate"] = "LIGHT";
    data["flags"] = { "STR", "DEX" };
    REQUIRE(BaseitemReader(data).read().has_value());
    REQUIRE(items.size() == 5);
    const auto &item = items.get_baseitem(4);
    CHECK(item.name == "Test");
    CHECK(item.get_symbol().character == '-');
    CHECK(item.bi_key.tval() == ItemKindType::ROD);
    CHECK(item.bi_key.sval() == 2);
    CHECK(item.pval == 9);
    CHECK(item.alloc_tables[0].level == 4);
    CHECK(item.alloc_tables[0].chance == 5);
    CHECK(item.alloc_tables[1].level == 6);
    CHECK(item.alloc_tables[1].chance == 7);
    CHECK(item.act_idx == RandomArtActType::LIGHT);
    CHECK(item.flags.has(TR_STR));
    CHECK(item.flags.has(TR_DEX));
    CHECK(item.flags.has(TR_ACTIVATE));
    CHECK(error_idx == 4);
}

/*!
 * @brief 有効なベースアイテムどうしで種別・副種別が重なると拒否する
 * @details 種別・副種別から ID を引く表では後の ID が先の ID を隠すため。名前が空か「異常アイテム」の
 * 無効なベースアイテムは表に載らないので、重なってもよい。
 */
TEST_CASE("BaseitemReader rejects duplicated kinds of valid baseitems")
{
    BaseitemStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const auto read = [](const nlohmann::json &data) { return BaseitemReader(data).read(); };
    REQUIRE(read(make_baseitem(1)).has_value());

    auto duplicated = make_baseitem(2);
    CHECK(test::to_error_code(read(duplicated)) == ParseErrorType::INVALID_VALUE);
    CHECK(items.size() == 2);

    // 失敗した読み込みでも診断のための ID は進むため、同じ ID を読み直す前に戻す。
    error_idx = 1;
    auto invalid = make_baseitem(2);
    // JSON の文字列は UTF-8 で与える（「異常アイテム」）。
    invalid["name"] = { { "ja", "\xe7\x95\xb0\xe5\xb8\xb8\xe3\x82\xa2\xe3\x82\xa4\xe3\x83\x86\xe3\x83\xa0" }, { "en", "Invalid item" } };
    REQUIRE(read(invalid).has_value());
    CHECK_FALSE(items.get_baseitem(2).is_valid());

    auto other_subtype = make_baseitem(3);
    other_subtype["itemkind"]["subtype_value"] = 2;
    REQUIRE(read(other_subtype).has_value());
    auto other_type = make_baseitem(4);
    other_type["itemkind"]["type_value"] = 2;
    REQUIRE(read(other_type).has_value());
    CHECK(test::to_error_code(read(make_baseitem(5))) == ParseErrorType::INVALID_VALUE);
    CHECK(items.size() == 5);
}

/*!
 * @brief ダイスの個数と面数は、セーブの1バイトに収まる 0〜255 に限る
 */
TEST_CASE("BaseitemReader limits base dice to the save format range")
{
    for (const auto *dice : { "256d1", "1d256", "-1d2" }) {
        CAPTURE(dice);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["base_dice"] = dice;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(BaseitemList::get_instance().empty());
    }
    BaseitemStateGuard guard;
    auto data = make_baseitem();
    data["base_dice"] = "255d255";
    REQUIRE(BaseitemReader(data).read().has_value());
    CHECK(BaseitemList::get_instance().get_baseitem(1).damage_dice == Dice(255, 255));
}

/*!
 * @brief item-reader-limits.h の見積もり (前提はそのファイルに書いた) の式に範囲の端の値を入れると、16ビットに収まる
 * @details 見積もりの式を確かめるだけで、合計があふれないことの保証ではない (修理を繰り返した場合などは含まない)。
 */
TEST_CASE("Item reader limits keep the stated equipment-sum estimate within 16 bits")
{
    constexpr auto max16 = int{ std::numeric_limits<int16_t>::max() };
    constexpr auto slots = 12;
    // ACの合計: ベースAC (エゴの XTRA_AC で 1/4 上乗せ) + 盾の技能 + ACの修正値 (修理した武器2か所は2倍) + そのほかの補正
    CHECK(slots * (ITEM_BASE_AC_MAX + ITEM_BASE_AC_MAX / 4) + 12 + (slots + 2) * (ITEM_BONUS_MAX + 70) + 800 <= max16);
    // 命中の合計 (弓の部位を含む12か所) とダメージの合計 (弓の部位を除く11か所)
    CHECK(slots * (ITEM_BONUS_MAX + 481) + 316 <= max16);
    CHECK((slots - 1) * (ITEM_BONUS_MAX + 481) + 652 <= max16);
}

/*!
 * @brief ベースACと修正値は絶対値が255以下の小さな数に限る (ベースACは負の値を認めない)
 */
TEST_CASE("BaseitemReader limits base AC and bonuses to small numbers up to 255 in absolute value")
{
    for (const auto &[field, value] : { std::pair{ "base_ac", -1 }, std::pair{ "base_ac", ITEM_BASE_AC_MAX + 1 }, std::pair{ "hit_bonus", -ITEM_BONUS_MAX - 1 },
             std::pair{ "hit_bonus", ITEM_BONUS_MAX + 1 }, std::pair{ "damage_bonus", -ITEM_BONUS_MAX - 1 }, std::pair{ "damage_bonus", ITEM_BONUS_MAX + 1 },
             std::pair{ "ac_bonus", -ITEM_BONUS_MAX - 1 }, std::pair{ "ac_bonus", ITEM_BONUS_MAX + 1 } }) {
        CAPTURE(field);
        CAPTURE(value);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data[field] = value;
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(BaseitemList::get_instance().empty());
    }
    for (const auto bonus : { -ITEM_BONUS_MAX, ITEM_BONUS_MAX }) {
        CAPTURE(bonus);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        const auto base_ac = bonus < 0 ? 0 : ITEM_BASE_AC_MAX;
        data["base_ac"] = base_ac;
        data["hit_bonus"] = bonus;
        data["damage_bonus"] = bonus;
        data["ac_bonus"] = bonus;
        REQUIRE(BaseitemReader(data).read().has_value());
        const auto &baseitem = BaseitemList::get_instance().get_baseitem(1);
        CHECK(baseitem.ac == base_ac);
        CHECK(baseitem.to_h == bonus);
        CHECK(baseitem.to_d == bonus);
        CHECK(baseitem.to_a == bonus);
    }
}

/*!
 * @brief 出現階層は、生成の表で階層ごとの配列 (要素数 MAX_DEPTH) の添え字になるため、MAX_DEPTH - 1 までに限る
 */
TEST_CASE("BaseitemReader limits allocation depth to the generation table size")
{
    for (const auto depth : { MAX_DEPTH, -1 }) {
        CAPTURE(depth);
        BaseitemStateGuard guard;
        auto data = make_baseitem();
        data["allocations"] = { { { "depth", depth }, { "rarity", 1 } } };
        CHECK(test::to_error_code(BaseitemReader(data).read()) == ParseErrorType::INVALID_FLAG);
        CHECK(BaseitemList::get_instance().empty());
    }
    BaseitemStateGuard guard;
    auto data = make_baseitem();
    data["allocations"] = { { { "depth", MAX_DEPTH - 1 }, { "rarity", 1 } } };
    REQUIRE(BaseitemReader(data).read().has_value());
    const auto &table = BaseitemList::get_instance().get_baseitem(1).alloc_tables[0];
    CHECK(table.level == MAX_DEPTH - 1);
    CHECK(table.chance == 1);
}
