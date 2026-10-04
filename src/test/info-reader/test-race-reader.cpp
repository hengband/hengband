#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-reader.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>

namespace test {
class MonraceListTestAccess {
public:
    MonraceListTestAccess()
        : monraces(MonraceList::get_instance())
        , reader_state(0)
    {
        monraces.monraces.swap(saved_monraces);
    }

    MonraceListTestAccess(const MonraceListTestAccess &) = delete;
    MonraceListTestAccess &operator=(const MonraceListTestAccess &) = delete;

    ~MonraceListTestAccess()
    {
        monraces.monraces.swap(saved_monraces);
    }

private:
    MonraceList &monraces;
    MonraceList::Container saved_monraces;
    ScopedReaderState reader_state;
};
}

namespace {
nlohmann::json make_monrace()
{
    return {
        { "id", 0 },
        { "name", { { "ja", "test" }, { "en", "test" } } },
        { "symbol", { { "character", "p" }, { "color", "White" } } },
        { "speed", 0 },
        { "hit_point", "1d1" },
        { "vision", 1 },
        { "armor_class", 0 },
        { "alertness", 0 },
        { "level", 0 },
        { "rarity", 1 },
        { "exp", 0 },
        { "skill", { { "probability", "1_IN_3" }, { "list", { "SHRIEK" } } } },
    };
}

MonraceDefinition &test_monrace()
{
    return MonraceList::get_instance().get_monrace(static_cast<MonraceId>(0));
}
}

TEST_CASE("RaceReader accepts every schema-valid skill probability")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (int denominator = 1; denominator <= 100; ++denominator) {
        CAPTURE(denominator);
        data["skill"]["probability"] = "1_IN_" + std::to_string(denominator);
        REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
        CHECK(test_monrace().freq_spell == 100 / denominator);
        CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    }
}

TEST_CASE("RaceReader rejects invalid skill probability denominators without throwing")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto *probability : { "1_IN_0", "1_IN_101", "1_IN_255", "1_IN_256", "1_IN_-1", "1_IN_x", "1_IN_3x", "1_IN_99999999999999999999", "1_IN_01", "1_IN_+3", "1_IN_ 3", "1_IN_3 ", "1_IN_" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_VALUE);
    }
    for (const auto *probability : { "", "bad", "1_IN", "2_IN_3", "1_OF_3", "1_IN_3_extra" }) {
        CAPTURE(probability);
        data["skill"]["probability"] = probability;
        CHECK(RaceReader(data).read() == PARSE_ERROR_GENERIC);
    }
}

TEST_CASE("RaceReader reports missing and nonstring skill probabilities")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"].erase("probability");
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["probability"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }
}

TEST_CASE("RaceReader reads shoot dice with and without a skill list")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["skill"]["shoot"] = "2d6";
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHOOT));
    CHECK(test_monrace().ability_flags.has(MonsterAbilityType::SHRIEK));
    data["skill"].erase("list");
    REQUIRE(RaceReader(data).read() == PARSE_ERROR_NONE);
    CHECK(test_monrace().shoot_damage_dice == Dice(2, 6));
    data["skill"].erase("shoot");
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
}

TEST_CASE("RaceReader rejects nonstring shoot dice and skill list entries")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["shoot"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["skill"]["shoot"] = nullptr;
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["skill"]["shoot"] = "bad";
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["skill"].erase("shoot");
    for (const auto &value : { nlohmann::json(), nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["skill"]["list"] = nlohmann::json::array({ "SHRIEK", value });
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["skill"]["list"] = { "UNKNOWN" };
    CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
}

TEST_CASE("RaceReader rejects nonstring message actions before message insertion")
{
    test::MonraceListTestAccess guard;
    auto data = make_monrace();
    data["message"] = nlohmann::json::array({ { { "chance", 100 } } });
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    data["message"][0]["action"] = nullptr;
    CHECK(RaceReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    for (const auto &value : { nlohmann::json(42), nlohmann::json(true), nlohmann::json::array(), nlohmann::json::object() }) {
        data["message"][0]["action"] = value;
        CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_TYPE);
    }
    data["message"][0]["action"] = "UNKNOWN";
    CHECK(RaceReader(data).read() == PARSE_ERROR_INVALID_FLAG);
}
