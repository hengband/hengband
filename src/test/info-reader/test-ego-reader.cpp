#include "artifact/random-art-effects.h"
#include "info-reader/ego-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/character-encoding.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace {
const std::string EGO_NAME_JA_UTF8 = "\xe8\xa9\xa6\xe9\xa8\x93\xe3\x81\xae";

class EgoStateGuard {
public:
    EgoStateGuard()
        : saved_error_idx(error_idx)
    {
        this->saved_egos.swap(egos_info);
        error_idx = -1;
    }

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
        { "extra_flags", { { { "numerator", 1 }, { "denominator", 3 }, { "flags", { "RES_FIRE", "HEAVY_CURSE" } } } } }
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
    CHECK(ego.xtra_flags[0].mul == 1);
    CHECK(ego.xtra_flags[0].dev == 3);
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
    data["extra_flags"][0]["denominator"] = 0;
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

TEST_CASE("EgoReader accepts levels throughout DEPTH storage")
{
    EgoStateGuard guard;
    auto data = make_ego();
    data["level"] = 32768;
    REQUIRE(EgoReader(data).read() == PARSE_ERROR_NONE);
    CHECK(egos_info.at(i2enum<EgoType>(4)).level == 32768);
}
