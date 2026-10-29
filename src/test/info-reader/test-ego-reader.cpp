#include "artifact/random-art-effects.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-ego-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <array>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace {
const std::string EGO_NAME_JA_UTF8 = "\xe8\xa9\xa6\xe9\xa8\x93\xe3\x81\xae";

struct EgoStateGuard {
    test::ScopedReaderState reader_state;
    test::ScopedEgoList egos;

    EgoStateGuard() = default;
    EgoStateGuard(const EgoStateGuard &) = delete;
    EgoStateGuard(EgoStateGuard &&) = delete;
    EgoStateGuard &operator=(const EgoStateGuard &) = delete;
    EgoStateGuard &operator=(EgoStateGuard &&) = delete;
};

/*!
 * @brief 数値・能力・発動を含む正常な整数百分率形式のエゴ定義を作る
 * @return 境界値テストで各項目を差し替えるためのJSON
 */
nlohmann::json make_ego()
{
    return {
        { "id", 4 },
        { "name", { { "ja", EGO_NAME_JA_UTF8 }, { "en", "of Testing" } } },
        { "slot", 31 },
        { "rating", 30 },
        { "level", 10 },
        { "rarity", 20 },
        { "cost", 12345 },
        { "base_bonuses", { { "to_hit", -3 }, { "to_damage", -2 }, { "to_ac", 1 } } },
        { "maximum_bonuses", { { "to_hit", 5 }, { "to_damage", 6 }, { "to_ac", 7 }, { "pval", 3 } } },
        { "activation", "BA_FIRE_4" },
        { "flags", { "STR", "XTRA_H_RES" } },
        { "extra_flags", { { { "chance", 33 }, { "flags", { "RES_FIRE", "HEAVY_CURSE" } } } } }
    };
}
}

TEST_CASE("EgoReader loads bonuses, flags, activation, and probabilistic flags")
{
    auto session = EgoReader::create_session();
    EgoStateGuard guard;
    session = EgoReader::create_session();
    const auto data = make_ego();
    REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
    REQUIRE(egos_info.size() == 1);
    const auto &ego = egos_info.at(i2enum<EgoType>(4));
#ifdef JP
    const auto name_utf8 = sys_to_utf8(ego.name);
    REQUIRE(name_utf8.has_value());
    CHECK(*name_utf8 == EGO_NAME_JA_UTF8);
#else
    CHECK(ego.name == "of Testing");
#endif
    CHECK(ego.slot == 31);
    CHECK(ego.rating == 30);
    CHECK(ego.level == 10);
    CHECK(ego.rarity == 20);
    CHECK(ego.cost == 12345);
    CHECK(ego.base_to_h == -3);
    CHECK(ego.base_to_d == -2);
    CHECK(ego.base_to_a == 1);
    CHECK(ego.max_to_h == 5);
    CHECK(ego.max_to_d == 6);
    CHECK(ego.max_to_a == 7);
    CHECK(ego.max_pval == 3);
    CHECK(ego.act_idx == RandomArtActType::BA_FIRE_4);
    CHECK(ego.flags.has(TR_STR));
    CHECK(ego.gen_flags.has(ItemGenerationTraitType::XTRA_H_RES));
    REQUIRE(ego.xtra_flags.size() == 1);
    CHECK(ego.xtra_flags[0].chance == 33);
    CHECK(ego.xtra_flags[0].tr_flags == std::vector<tr_type>{ TR_RES_FIRE });
    CHECK(ego.xtra_flags[0].trg_flags == std::vector<ItemGenerationTraitType>{ ItemGenerationTraitType::HEAVY_CURSE });
}

TEST_CASE("EgoReader rejects malformed records atomically")
{
    auto session = EgoReader::create_session();
    EgoStateGuard guard;
    session = EgoReader::create_session();
    const auto original = make_ego();
    auto check_invalid = [&](const nlohmann::json &data) {
        CHECK_FALSE(test::read_and_commit(EgoReader(data, session), session).has_value());
        CHECK(egos_info.empty());
        CHECK(session.last_id().value_or(-1) == -1);
    };

    auto data = original;
    data.erase("name");
    check_invalid(data);
    data = original;
    data["slot"] = "31";
    check_invalid(data);
    data = original;
    data["slot"] = 256;
    check_invalid(data);
    data = original;
    data["rarity"] = 256;
    check_invalid(data);
    data = original;
    data["base_bonuses"]["to_hit"] = 32768;
    check_invalid(data);
    data = original;
    data["maximum_bonuses"].erase("pval");
    check_invalid(data);
}

TEST_CASE("EgoReader requires unique IDs while preserving legacy ordering")
{
    auto session = EgoReader::create_session();
    EgoStateGuard guard;
    session = EgoReader::create_session();
    auto data = make_ego();
    REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
    CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == ParseErrorType::NON_SEQUENTIAL_RECORDS);
    data["id"] = 3;
    CHECK(test::read_and_commit(EgoReader(data, session), session).has_value());
    data["id"] = 5;
    CHECK(test::read_and_commit(EgoReader(data, session), session).has_value());
    CHECK(session.last_id().value_or(-1) == 5);
    CHECK(egos_info.size() == 3);
}

/*!
 * @brief 発動の基本難易度の level は、ベースアイテム・アーティファクトの level と同じく MAX_DEPTH - 1 までとする
 */
TEST_CASE("EgoReader limits the activation difficulty level like item levels")
{
    auto session = EgoReader::create_session();
    {
        EgoStateGuard guard;
        session = EgoReader::create_session();
        auto data = make_ego();
        data["level"] = MAX_DEPTH;
        CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == ParseErrorType::INVALID_VALUE);
        CHECK_FALSE(egos_info.contains(i2enum<EgoType>(4)));
    }
    EgoStateGuard guard;
    session = EgoReader::create_session();
    auto data = make_ego();
    data["level"] = MAX_DEPTH - 1;
    REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
    CHECK(egos_info.at(i2enum<EgoType>(4)).level == MAX_DEPTH - 1);
}

TEST_CASE("EgoReader accepts integer percent endpoints")
{
    auto session = EgoReader::create_session();
    for (const auto chance : { 0, 1, 33, 99, 100 }) {
        EgoStateGuard guard;
        session = EgoReader::create_session();
        auto data = make_ego();
        data["extra_flags"][0]["chance"] = chance;
        REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
        CHECK(egos_info.at(i2enum<EgoType>(4)).xtra_flags.at(0).chance == chance);
    }
}

TEST_CASE("EgoReader rejects invalid percentages and preserves published records")
{
    auto session = EgoReader::create_session();
    EgoStateGuard guard;
    session = EgoReader::create_session();
    const auto original = make_ego();
    REQUIRE(test::read_and_commit(EgoReader(original, session), session).has_value());
    const auto *published = &egos_info.at(i2enum<EgoType>(4));
    const auto check_invalid = [&](const nlohmann::json &extra) {
        auto data = original;
        data["id"] = 5;
        data["extra_flags"][0] = extra;
        CHECK_FALSE(test::read_and_commit(EgoReader(data, session), session).has_value());
        REQUIRE(egos_info.size() == 1);
        CHECK(&egos_info.at(i2enum<EgoType>(4)) == published);
        CHECK(published->xtra_flags.at(0).chance == 33);
        CHECK(session.last_id().value_or(-1) == 4);
    };
    for (const auto &chance : std::array<nlohmann::json, 4>{ { -1, uint64_t{ 2147483648 }, std::numeric_limits<uint64_t>::max(), 33.0 } }) {
        auto extra = original["extra_flags"][0];
        extra["chance"] = chance;
        check_invalid(extra);
    }
    auto extra = original["extra_flags"][0];
    extra.erase("chance");
    check_invalid(extra);
}

TEST_CASE("EgoReader validates the percent document version")
{
    auto session = EgoReader::create_session();
    const nlohmann::json valid{ { "version", 2 } };
    CHECK(EgoReader::validate_root(valid).has_value());
    for (const auto &version : std::array<nlohmann::json, 8>{ { 1, 3, -1, 2.0, true, "2", nullptr, std::numeric_limits<uint64_t>::max() } }) {
        auto root = valid;
        root["version"] = version;
        CHECK_FALSE(EgoReader::validate_root(root).has_value());
    }
    auto root = valid;
    root.erase("version");
    CHECK_FALSE(EgoReader::validate_root(root).has_value());
    CHECK_FALSE(EgoReader::validate_root(nlohmann::json::array()).has_value());
}

TEST_CASE("EgoReader restricts IDs to save format and slots to equipment or ammo")
{
    auto session = EgoReader::create_session();
    for (const auto id : { 1, 255 }) {
        for (int slot = 23; slot <= 35; ++slot) {
            EgoStateGuard guard;
            session = EgoReader::create_session();
            auto data = make_ego();
            data["id"] = id;
            data["slot"] = slot;
            REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
            CHECK(egos_info.at(i2enum<EgoType>(id)).slot == slot);
        }
    }
    for (const auto &[key, value] : std::array{
             std::pair{ "id", 0 }, std::pair{ "id", 256 }, std::pair{ "id", 32767 },
             std::pair{ "slot", 22 }, std::pair{ "slot", 36 }, std::pair{ "slot", 0 }, std::pair{ "slot", 255 } }) {
        EgoStateGuard guard;
        session = EgoReader::create_session();
        auto data = make_ego();
        data[key] = value;
        CHECK_FALSE(test::read_and_commit(EgoReader(data, session), session).has_value());
        CHECK(egos_info.empty());
        CHECK(session.last_id().value_or(-1) == -1);
    }
}

TEST_CASE("EgoReader checks every scalar and signed bonus boundary")
{
    auto session = EgoReader::create_session();
    const std::array paths{ "/rating", "/level", "/cost", "/rarity", "/base_bonuses/to_hit", "/base_bonuses/to_damage", "/base_bonuses/to_ac",
        "/maximum_bonuses/to_hit", "/maximum_bonuses/to_damage", "/maximum_bonuses/to_ac", "/maximum_bonuses/pval" };
    for (const auto *path : paths) {
        const auto bonus = std::string_view(path).find("bonuses") != std::string_view::npos;
        const auto low = bonus ? -32768 : 0;
        const auto high = bonus ? 32767 : std::string_view(path) == "/rarity" ? 255
                                      : std::string_view(path) == "/cost"     ? std::numeric_limits<PRICE>::max() / 32
                                      : std::string_view(path) == "/rating"   ? 100
                                                                              : MAX_DEPTH - 1;
        for (const auto value : { int64_t{ low } - 1, int64_t{ low }, int64_t{ high }, int64_t{ high } + 1 }) {
            EgoStateGuard guard;
            session = EgoReader::create_session();
            auto data = make_ego();
            data[nlohmann::json::json_pointer(path)] = value;
            const auto result = test::read_and_commit(EgoReader(data, session), session);
            CHECK(result.has_value() == (value >= low && value <= high));
        }
    }
}

/*!
 * @brief 内部ヘルパーの各エラーと優先順位を維持し、既存定義と読込位置を保持する
 */
TEST_CASE("EgoReader propagates helper errors without publishing partial records")
{
    auto session = EgoReader::create_session();
    EgoStateGuard guard;
    session = EgoReader::create_session();
    const auto original = make_ego();
    REQUIRE(test::read_and_commit(EgoReader(original, session), session).has_value());
    const auto *published = &egos_info.at(i2enum<EgoType>(4));
    const auto check_invalid = [&](const nlohmann::json &data, ParseErrorType expected) {
        CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == expected);
        REQUIRE(egos_info.size() == 1);
        CHECK_FALSE(egos_info.contains(i2enum<EgoType>(5)));
        CHECK(&egos_info.at(i2enum<EgoType>(4)) == published);
        CHECK(published->cost == 12345);
        CHECK(published->act_idx == RandomArtActType::BA_FIRE_4);
        CHECK(published->flags.has(TR_STR));
        CHECK(published->gen_flags.has(ItemGenerationTraitType::XTRA_H_RES));
        REQUIRE(published->xtra_flags.size() == 1);
        CHECK(published->xtra_flags[0].chance == 33);
        CHECK(session.last_id().value_or(-1) == 4);
    };
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        ParseErrorType error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/activation", 1, ParseErrorType::INVALID_TYPE },
        { "/activation", "UNKNOWN", ParseErrorType::INVALID_FLAG },
        { "/activation", "999999999999999999999999999999", ParseErrorType::INVALID_FLAG },
        { "/activation", "0", ParseErrorType::INVALID_FLAG },
        { "/flags", 1, ParseErrorType::INVALID_TYPE },
        { "/flags", nlohmann::json::array({ "STR", 1 }), ParseErrorType::INVALID_TYPE },
        { "/flags", nlohmann::json::array({ "STR", "UNKNOWN" }), ParseErrorType::INVALID_FLAG },
        { "/extra_flags", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/extra_flags/0", 1, ParseErrorType::INVALID_TYPE },
        { "/extra_flags/1", 1, ParseErrorType::INVALID_TYPE },
        { "/extra_flags/0/numerator", 1, ParseErrorType::INVALID_VALUE },
        { "/extra_flags/0/denominator", 3, ParseErrorType::INVALID_VALUE },
        { "/extra_flags/0", { { "numerator", 1 }, { "denominator", 3 }, { "flags", { "RES_FIRE" } } }, ParseErrorType::INVALID_VALUE },
        { "/extra_flags/0/chance", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/extra_flags/0/chance", false, ParseErrorType::INVALID_TYPE },
        { "/extra_flags/0/chance", 101, ParseErrorType::INVALID_VALUE },
        { "/extra_flags/0/flags", nullptr, ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/extra_flags/0/flags", nlohmann::json::object(), ParseErrorType::INVALID_TYPE },
        { "/extra_flags/0/flags", nlohmann::json::array(), ParseErrorType::TOO_FEW_ARGUMENTS },
        { "/extra_flags/0/flags", nlohmann::json::array({ "RES_FIRE", 1 }), ParseErrorType::INVALID_TYPE },
        { "/extra_flags/0/flags", nlohmann::json::array({ "RES_FIRE", "UNKNOWN" }), ParseErrorType::INVALID_FLAG },
    };
    for (const auto &invalid : invalid_fields) {
        CAPTURE(invalid.path);
        CAPTURE(invalid.value.dump());
        auto data = original;
        data["id"] = 5;
        data[nlohmann::json::json_pointer(invalid.path)] = invalid.value;
        check_invalid(data, invalid.error);
    }

    auto data = original;
    data["id"] = 5;
    data["activation"] = "UNKNOWN";
    data["flags"] = 1;
    data["extra_flags"][0]["numerator"] = 1;
    check_invalid(data, ParseErrorType::INVALID_FLAG);
    data["activation"] = "BA_FIRE_4";
    check_invalid(data, ParseErrorType::INVALID_TYPE);
    data["flags"] = original["flags"];
    check_invalid(data, ParseErrorType::INVALID_VALUE);
}

/*!
 * @brief 任意の発動・フラグ・追加能力を省略またはnullにしたときの正常値を維持する
 */
TEST_CASE("EgoReader accepts absent optional activation and flag groups")
{
    auto session = EgoReader::create_session();
    for (const auto use_null : { false, true }) {
        CAPTURE(use_null);
        EgoStateGuard guard;
        session = EgoReader::create_session();
        auto data = make_ego();
        for (const auto *key : { "activation", "flags", "extra_flags" }) {
            if (use_null) {
                data[key] = nullptr;
            } else {
                data.erase(key);
            }
        }
        REQUIRE(test::read_and_commit(EgoReader(data, session), session).has_value());
        REQUIRE(egos_info.size() == 1);
        const auto &ego = egos_info.at(i2enum<EgoType>(4));
        CHECK(ego.act_idx == RandomArtActType::NONE);
        CHECK_FALSE(ego.flags.has(TR_STR));
        CHECK_FALSE(ego.gen_flags.has(ItemGenerationTraitType::XTRA_H_RES));
        CHECK(ego.xtra_flags.empty());
        CHECK(session.last_id().value_or(-1) == 4);
    }
}

TEST_CASE("EgoReader permits reloading the same ID in a new session")
{
    EgoStateGuard guard;
    auto session = EgoReader::create_session();
    auto data = make_ego();
    REQUIRE(test::read_and_commit(EgoReader(data, session), session));
    CHECK_FALSE(test::read_and_commit(EgoReader(data, session), session));
    auto reloaded = EgoReader::create_session();
    data["cost"] = 7;
    REQUIRE(test::read_and_commit(EgoReader(data, reloaded), reloaded));
    CHECK(egos_info.at(i2enum<EgoType>(4)).cost == 7);
}

TEST_CASE("EgoReader pins flag array type errors and missing values")
{
    for (const auto *path : { "/flags", "/extra_flags/0/flags" }) {
        for (const auto *input : { "42", "[1]", "null", "missing", "[]" }) {
            CAPTURE(path);
            CAPTURE(input);
            EgoStateGuard guard;
            auto session = EgoReader::create_session();
            auto data = make_ego();
            const auto pointer = nlohmann::json::json_pointer(path);
            const std::string input_text = input;
            if (input_text == "missing") {
                data[pointer.parent_pointer()].erase(pointer.back());
            } else {
                data[pointer] = nlohmann::json::parse(input);
            }
            auto expected = ParseErrorType::NONE;
            if (input_text == "42") {
                expected = ParseErrorType::INVALID_TYPE;
            } else if (input_text == "[1]") {
                expected = ParseErrorType::INVALID_TYPE;
            }
            if (std::string(path) == "/extra_flags/0/flags" && (input_text == "null" || input_text == "missing" || input_text == "[]")) {
                expected = ParseErrorType::TOO_FEW_ARGUMENTS;
            }
            CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == expected);
        }
    }
}

TEST_CASE("EgoReader rejects an unknown flag without publishing the record")
{
    EgoStateGuard guard;
    auto session = EgoReader::create_session();
    auto data = make_ego();
    data["flags"] = { "UNKNOWN_EGO_FLAG" };
    ScopedMessageCapture diagnostics;
    CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == ParseErrorType::INVALID_FLAG);
    CHECK(egos_info.empty());
    CHECK_FALSE(session.last_id().has_value());
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0] == _("未知のエゴ・フラグ 'UNKNOWN_EGO_FLAG'。", "Unknown ego flag 'UNKNOWN_EGO_FLAG'."));
}

TEST_CASE("EgoReader rejects an unknown extra flag without publishing the record")
{
    EgoStateGuard guard;
    auto session = EgoReader::create_session();
    auto data = make_ego();
    data["extra_flags"][0]["flags"] = { "RES_FIRE", "UNKNOWN_EXTRA_FLAG" };
    ScopedMessageCapture diagnostics;
    CHECK(test::to_error_code(test::read_and_commit(EgoReader(data, session), session)) == ParseErrorType::INVALID_FLAG);
    CHECK(egos_info.empty());
    CHECK_FALSE(session.last_id().has_value());
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages()[0] == _("未知のエゴ・フラグ 'UNKNOWN_EXTRA_FLAG'。", "Unknown ego flag 'UNKNOWN_EXTRA_FLAG'."));
}
