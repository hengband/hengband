#include "info-reader/parse-error-types.h"
#include "info-reader/wilderness-reader.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

namespace {
const std::string TOWN_NAME_JA_UTF8 = "\xe7\x94\xba";

nlohmann::json make_definition()
{
    return {
        { "version", 1 },
        { "width", 3 },
        { "height", 3 },
        { "towns", { { { "id", 1 }, { "name", { { "ja", TOWN_NAME_JA_UTF8 }, { "en", "Town" } } } } } },
        { "maps", {
                      { "normal", {
                                      { "letters", {
                                                       { { "symbol", "#" }, { "terrain", 0 } },
                                                       { { "symbol", "1" }, { "terrain", 1 }, { "level", { { "ja", 30 }, { "en", 20 } } }, { "town", 1 } },
                                                   } },
                                      { "layout", { "###", "#1#", "###" } },
                                      { "starting_position", { { "x", 1 }, { "y", 1 } } },
                                  } },
                      { "compact", {
                                       { "letters", { { { "symbol", "#" }, { "terrain", 0 } } } },
                                       { "layout", { "###", "###", "###" } },
                                       { "starting_position", { { "x", 1 }, { "y", 1 } } },
                                   } },
                  } },
    };
}

WildernessDefinition make_existing_definition()
{
    WildernessDefinition definition;
    definition.width = 99;
    definition.height = 88;
    definition.towns.push_back({ 77, "Previous town", "Previous alias" });
    definition.normal.letters.push_back({ '?', WildernessTerrain::EDGE, 5, 0, 0 });
    definition.normal.layout.push_back("previous normal map");
    definition.compact.layout.push_back("previous compact map");
    return definition;
}
}

TEST_CASE("WildernessReader loads maps and localized levels")
{
    const auto data = make_definition();
    WildernessDefinition definition;
    REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
    CHECK(definition.width == 3);
    CHECK(definition.height == 3);
    REQUIRE(definition.towns.size() == 1);
    REQUIRE(definition.normal.letters.size() == 2);
    CHECK(definition.normal.letters[1].terrain == WildernessTerrain::TOWN);
    const auto expected_level = definition.towns[0].name == "Town" ? 20 : 30;
    CHECK(definition.normal.letters[1].level == expected_level);
    CHECK(definition.normal.letters[1].town == 1);
    CHECK(definition.normal.starting_position == Pos2D(1, 1));
}

TEST_CASE("WildernessReader rejects duplicate IDs and symbols")
{
    auto data = make_definition();
    data["towns"].push_back(data["towns"][0]);
    WildernessDefinition definition;
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);

    data = make_definition();
    data["maps"]["normal"]["letters"].push_back(data["maps"]["normal"]["letters"][0]);
    definition = {};
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
}

TEST_CASE("WildernessReader rejects undefined town references")
{
    for (const auto *map_name : { "normal", "compact" }) {
        auto data = make_definition();
        data["maps"][map_name]["letters"][0]["town"] = 2;
        WildernessDefinition definition;
        CAPTURE(map_name);
        CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("WildernessReader validates layout dimensions and symbols")
{
    auto data = make_definition();
    data["maps"]["normal"]["layout"][1] = "##";
    WildernessDefinition definition;
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);

    data = make_definition();
    data["maps"]["normal"]["layout"][1] = "#.#";
    definition = {};
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);

    data = make_definition();
    data["maps"]["compact"]["starting_position"]["x"] = 3;
    definition = {};
    CHECK(WildernessReader(data).read(definition) != PARSE_ERROR_NONE);
}

TEST_CASE("WildernessReader keeps previous output on a late parse error")
{
    auto data = make_definition();
    data["maps"]["compact"]["starting_position"]["x"] = 3;
    auto definition = make_existing_definition();

    CHECK(WildernessReader(data).read(definition) != PARSE_ERROR_NONE);
    CHECK(definition.width == 99);
    CHECK(definition.height == 88);
    REQUIRE(definition.towns.size() == 1);
    CHECK(definition.towns[0].id == 77);
    CHECK(definition.towns[0].name == "Previous town");
    CHECK(definition.towns[0].alias == "Previous alias");
    REQUIRE(definition.normal.letters.size() == 1);
    CHECK(definition.normal.letters[0].symbol == '?');
    REQUIRE(definition.normal.layout.size() == 1);
    CHECK(definition.normal.layout[0] == "previous normal map");
    REQUIRE(definition.compact.layout.size() == 1);
    CHECK(definition.compact.layout[0] == "previous compact map");
}

TEST_CASE("WildernessReader replaces previous output on success")
{
    const auto data = make_definition();
    auto definition = make_existing_definition();

    REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
    CHECK(definition.width == 3);
    CHECK(definition.height == 3);
    REQUIRE(definition.towns.size() == 1);
    CHECK(definition.towns[0].id == 1);
    REQUIRE(definition.normal.letters.size() == 2);
    REQUIRE(definition.normal.layout.size() == 3);
    CHECK(definition.normal.layout[0] == "###");
    CHECK(definition.normal.layout[1] == "#1#");
    REQUIRE(definition.compact.layout.size() == 3);
    CHECK(definition.compact.layout[0] == "###");
}
