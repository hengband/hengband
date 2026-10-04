#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/artifact-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "object-enchant/tr-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/system/artifact-list-test-access.h"
#include "util/dice.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace {
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
        auto data = make_artifact();
        data["id"] = id;
        CHECK(ArtifactReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(ArtifactList::get_instance().empty());
        CHECK(error_idx == -1);
    }
}

TEST_CASE("ArtifactReader preserves missing and non-integer ID errors")
{
    for (const auto &id : { nlohmann::json(), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        auto data = make_artifact();
        data["id"] = id;
        CHECK(ArtifactReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
        CHECK(error_idx == -1);
        CHECK(ArtifactList::get_instance().empty());
    }
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    auto data = make_artifact();
    data.erase("id");
    CHECK(ArtifactReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    const auto non_object = nlohmann::json::array();
    CHECK(ArtifactReader(non_object).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    CHECK(error_idx == -1);
    CHECK(ArtifactList::get_instance().empty());
}

TEST_CASE("ArtifactReader accepts schema ID boundaries without changing the ordering check")
{
    for (const auto &id : { nlohmann::json(0), nlohmann::json(9999), nlohmann::json(nlohmann::json::number_unsigned_t{ 0 }), nlohmann::json(nlohmann::json::number_unsigned_t{ 9999 }) }) {
        CAPTURE(id);
        test::ScopedReaderState reader_state;
        test::ArtifactListTestAccess artifact_state;
        auto data = make_artifact();
        data["id"] = id;
        REQUIRE(ArtifactReader(data).read() == PARSE_ERROR_NONE);
        auto &artifacts = ArtifactList::get_instance();
        REQUIRE(artifacts.size() == 1);
        CHECK(enum2i(artifacts.begin()->first) == id.get<int>());
        CHECK(error_idx == id.get<int>());
        CHECK(ArtifactReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
        CHECK(artifacts.size() == 1);
    }
}

TEST_CASE("ArtifactReader rejects descending IDs without publishing another record")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
    const auto original = make_artifact(4);
    const auto descending = make_artifact(3);
    REQUIRE(ArtifactReader(original).read() == PARSE_ERROR_NONE);
    CHECK(ArtifactReader(descending).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
    CHECK(ArtifactList::get_instance().size() == 1);
    CHECK(error_idx == 4);
}

TEST_CASE("ArtifactReader publishes complete records with localized text and optional fields")
{
    test::ScopedReaderState reader_state;
    test::ArtifactListTestAccess artifact_state;
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
    REQUIRE(ArtifactReader(data).read() == PARSE_ERROR_NONE);
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
    const auto data = make_artifact();
    REQUIRE(ArtifactReader(data).read() == PARSE_ERROR_NONE);
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
    auto &artifacts = ArtifactList::get_instance();
    const auto original_data = make_artifact();
    REQUIRE(ArtifactReader(original_data).read() == PARSE_ERROR_NONE);
    const auto *original = std::addressof(artifacts.get_artifact(i2enum<FixedArtifactId>(1)));
    auto data = make_artifact(2);
    data["activate"] = "BA_FIRE_4";
    data["flags"] = { "STR" };
    auto expected_error = PARSE_ERROR_INVALID_TYPE;
    SUBCASE("early name error")
    {
        data["name"] = 42;
    }
    SUBCASE("late flag error after a valid flag and activation")
    {
        data["flags"] = { "STR", "UNKNOWN_FLAG" };
        expected_error = PARSE_ERROR_INVALID_FLAG;
    }
    SUBCASE("last flavor error after all other fields")
    {
        data["flavor"] = 42;
    }
    for (const auto id : { 1, 2 }) {
        CAPTURE(id);
        error_idx = -1;
        data["id"] = id;
        CHECK(ArtifactReader(data).read() == expected_error);
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
