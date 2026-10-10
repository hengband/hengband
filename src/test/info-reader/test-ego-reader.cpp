#include "artifact/random-art-effects.h"
#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/character-encoding.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "util/enum-converter.h"
#include <array>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
const std::string EGO_NAME_JA_UTF8 = "\xe8\xa9\xa6\xe9\xa8\x93\xe3\x81\xae";

class EgoStateGuard {
public:
    /*!
     * @brief 既存のエゴ定義とエラー位置を退避し、空の読込先で検証を始める
     */
    EgoStateGuard()
        : saved_error_idx(error_idx)
    {
        this->saved_egos.swap(egos_info);
        error_idx = -1;
    }

    /*!
     * @brief 検証中に公開された定義を破棄し、退避した定義とエラー位置を復元する
     */
    ~EgoStateGuard()
    {
        egos_info.clear();
        egos_info.swap(this->saved_egos);
        error_idx = this->saved_error_idx;
    }

private:
    std::map<EgoType, EgoItemDefinition> saved_egos;
    int saved_error_idx;
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
    EgoStateGuard guard;
    const auto data = make_ego();
    REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
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
    EgoStateGuard guard;
    const auto original = make_ego();
    auto check_invalid = [&](const nlohmann::json &data) {
        CHECK(EgoReader(data).read() != PARSE_ERROR_NONE);
        CHECK(egos_info.empty());
        CHECK(error_idx == -1);
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
    data["activation"] = "UNKNOWN";
    check_invalid(data);
    data = original;
    data["flags"] = { "UNKNOWN" };
    check_invalid(data);
    data = original;
    data["flags"] = { 1 };
    check_invalid(data);
    data = original;
    data["maximum_bonuses"].erase("pval");
    check_invalid(data);
    data = original;
    data["extra_flags"] = { 1 };
    check_invalid(data);
    data = original;
    data["extra_flags"][0]["chance"] = 101;
    check_invalid(data);
    data = original;
    data["extra_flags"][0]["flags"] = nlohmann::json::array();
    check_invalid(data);
}

TEST_CASE("EgoReader requires unique IDs while preserving legacy ordering")
{
    EgoStateGuard guard;
    auto data = make_ego();
    REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
    CHECK(EgoReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    data["id"] = 3;
    CHECK(EgoReader(data).read() == PARSE_ERROR_NONE);
    data["id"] = 5;
    CHECK(EgoReader(data).read() == PARSE_ERROR_NONE);
    CHECK(error_idx == 5);
    CHECK(egos_info.size() == 3);
}

TEST_CASE("EgoReader accepts the activation difficulty upper bound")
{
    EgoStateGuard guard;
    auto data = make_ego();
    data["level"] = 128;
    REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
    CHECK(egos_info.at(i2enum<EgoType>(4)).level == 128);
}

TEST_CASE("EgoReader accepts integer percent endpoints")
{
    for (const auto chance : { 0, 1, 33, 99, 100 }) {
        EgoStateGuard guard;
        auto data = make_ego();
        data["extra_flags"][0]["chance"] = chance;
        REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
        CHECK(egos_info.at(i2enum<EgoType>(4)).xtra_flags.at(0).chance == chance);
    }
}

TEST_CASE("EgoReader rejects invalid percentages and preserves published records")
{
    EgoStateGuard guard;
    const auto original = make_ego();
    REQUIRE(EgoReader(original).read() == PARSE_ERROR_NONE);
    const auto *published = &egos_info.at(i2enum<EgoType>(4));
    const auto check_invalid = [&](const nlohmann::json &extra) {
        auto data = original;
        data["id"] = 5;
        data["extra_flags"][0] = extra;
        CHECK(EgoReader(data).read() != PARSE_ERROR_NONE);
        REQUIRE(egos_info.size() == 1);
        CHECK(&egos_info.at(i2enum<EgoType>(4)) == published);
        CHECK(published->xtra_flags.at(0).chance == 33);
        CHECK(error_idx == 4);
    };
    for (const auto &chance : std::array<nlohmann::json, 7>{ { -1, 101, uint64_t{ 2147483648 }, std::numeric_limits<uint64_t>::max(), 33.0, false, nullptr } }) {
        auto extra = original["extra_flags"][0];
        extra["chance"] = chance;
        check_invalid(extra);
    }
    auto extra = original["extra_flags"][0];
    extra.erase("chance");
    check_invalid(extra);
    extra["numerator"] = 1;
    extra["denominator"] = 3;
    check_invalid(extra);
    extra["chance"] = 33;
    check_invalid(extra);
}

TEST_CASE("EgoReader validates the percent document version")
{
    const nlohmann::json valid{ { "version", 2 } };
    CHECK(EgoReader::validate_root(valid) == PARSE_ERROR_NONE);
    for (const auto &version : std::array<nlohmann::json, 8>{ { 1, 3, -1, 2.0, true, "2", nullptr, std::numeric_limits<uint64_t>::max() } }) {
        auto root = valid;
        root["version"] = version;
        CHECK(EgoReader::validate_root(root) != PARSE_ERROR_NONE);
    }
    auto root = valid;
    root.erase("version");
    CHECK(EgoReader::validate_root(root) != PARSE_ERROR_NONE);
    CHECK(EgoReader::validate_root(nlohmann::json::array()) != PARSE_ERROR_NONE);
}

TEST_CASE("EgoReader restricts IDs to save format and slots to equipment or ammo")
{
    for (const auto id : { 1, 255 }) {
        for (int slot = 23; slot <= 35; ++slot) {
            EgoStateGuard guard;
            auto data = make_ego();
            data["id"] = id;
            data["slot"] = slot;
            REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
            CHECK(egos_info.at(i2enum<EgoType>(id)).slot == slot);
        }
    }
    for (const auto &[key, value] : std::array{
             std::pair{ "id", 0 }, std::pair{ "id", 256 }, std::pair{ "id", 32767 },
             std::pair{ "slot", 22 }, std::pair{ "slot", 36 }, std::pair{ "slot", 0 }, std::pair{ "slot", 255 } }) {
        EgoStateGuard guard;
        auto data = make_ego();
        data[key] = value;
        CHECK(EgoReader(data).read() != PARSE_ERROR_NONE);
        CHECK(egos_info.empty());
        CHECK(error_idx == -1);
    }
}

TEST_CASE("EgoReader checks every scalar and signed bonus boundary")
{
    const std::array paths{ "/rating", "/level", "/cost", "/rarity", "/base_bonuses/to_hit", "/base_bonuses/to_damage", "/base_bonuses/to_ac",
        "/maximum_bonuses/to_hit", "/maximum_bonuses/to_damage", "/maximum_bonuses/to_ac", "/maximum_bonuses/pval" };
    for (const auto *path : paths) {
        const auto bonus = std::string_view(path).find("bonuses") != std::string_view::npos;
        const auto low = bonus ? -32768 : 0;
        const auto high = bonus ? 32767 : std::string_view(path) == "/rarity" ? 255
                                      : std::string_view(path) == "/cost"     ? std::numeric_limits<PRICE>::max() / 32
                                      : std::string_view(path) == "/rating"   ? 100
                                                                              : 128;
        for (const auto value : { int64_t{ low } - 1, int64_t{ low }, int64_t{ high }, int64_t{ high } + 1 }) {
            EgoStateGuard guard;
            auto data = make_ego();
            data[nlohmann::json::json_pointer(path)] = value;
            const auto result = EgoReader(data).read();
            CHECK((result == PARSE_ERROR_NONE) == (value >= low && value <= high));
        }
    }
}
