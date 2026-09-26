#include "info-reader/town-map-reader.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>

namespace {
nlohmann::json make_town_map()
{
    return {
        { "version", 2 },
        { "featureRules", { { { "symbol", "#" }, { "definition", { { "terrain", "WALL" }, { "caveInfo", 0 }, { "monster", "0" }, { "object", "0" }, { "ego", "0" }, { "artifact", "0" }, { "trap", "NONE" }, { "special", 0 } } } } } },
        { "buildingRules", { { { "index", 1 }, { "locale", "en" }, { "command", "N" }, { "fields", { "Building", "Owner", "Race" } } } } },
        { "mapVariants", { { { "rows", { "###", "#.#", "###" } } } } },
        { "startingPositions", { { { "y", 1 }, { "x", 1 } } } },
    };
}

parse_error_type read_town(const nlohmann::json &data, bool only_buildings = false)
{
    TownMapDefinition definition;
    return TownMapReader(data).read(definition, 3, 3, only_buildings);
}
}

TEST_CASE("TownMapReader retains ordered definitions, conditions and UTF-8 text")
{
    auto data = make_town_map();
    for (const auto *field : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        data[field][0]["when"] = "[EQU $TOWN 1]";
        data[field].push_back(data[field][0]);
        data[field][1].erase("when");
    }
    data["buildingRules"][0]["locale"] = "ja";
    data["buildingRules"][0]["fields"][0] = "町の建物";
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    REQUIRE(definition.features.size() == 2);
    CHECK(definition.features[0].symbol == '#');
    CHECK(definition.features[0].terrain == "WALL");
    CHECK(definition.features[0].monster == "0");
    CHECK(definition.features[0].object == "0");
    CHECK(definition.features[0].ego == "0");
    CHECK(definition.features[0].artifact == "0");
    CHECK(definition.features[0].trap == "NONE");
    CHECK(definition.features[0].cave_info == 0);
    CHECK(definition.features[0].special == 0);
    CHECK(definition.features[0].condition == "[EQU $TOWN 1]");
    CHECK_FALSE(definition.features[1].condition.has_value());
    REQUIRE(definition.buildings.size() == 2);
    CHECK(definition.buildings[0].index == 1);
    CHECK_FALSE(definition.buildings[0].english);
    CHECK(definition.buildings[1].english);
    CHECK(definition.buildings[0].command == 'N');
    CHECK(definition.buildings[0].fields[0] == "町の建物");
    CHECK(definition.buildings[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.maps.size() == 2);
    CHECK(definition.maps[0].rows == std::vector<std::string>{ "###", "#.#", "###" });
    CHECK(definition.maps[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.starts.size() == 2);
    CHECK(definition.starts[0].y == 1);
    CHECK(definition.starts[0].x == 1);
    CHECK(definition.starts[0].condition == "[EQU $TOWN 1]");
}

TEST_CASE("TownMapReader replaces output only after complete validation")
{
    auto data = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    data["featureRules"][0]["symbol"] = ".";
    data["startingPositions"][0]["x"] = 3;
    CHECK(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_INVALID_VALUE);
    CHECK(definition.features[0].symbol == '#');
    CHECK(definition.starts[0].x == 1);
    data["startingPositions"][0]["x"] = 2;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    CHECK(definition.features.size() == 1);
    CHECK(definition.features[0].symbol == '.');
    CHECK(definition.starts[0].x == 2);
}

TEST_CASE("TownMapReader validates root and conditions on every rule")
{
    for (const auto *field : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        auto data = make_town_map();
        data.erase(field);
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        data = make_town_map();
        data[field] = nullptr;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        for (const auto &condition : { nlohmann::json(nullptr), nlohmann::json(false), nlohmann::json("") }) {
            data = make_town_map();
            data[field][0]["when"] = condition;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        }
    }
    for (const auto &version : { nlohmann::json(1), nlohmann::json(2.0), nlohmann::json("2") }) {
        auto data = make_town_map();
        data["version"] = version;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto *field : { "mapVariants", "startingPositions" }) {
        auto data = make_town_map();
        data[field] = nlohmann::json::array();
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("TownMapReader validates feature token grammar and narrowing bounds")
{
    for (const auto *field : { "monster", "object", "ego", "artifact" }) {
        for (const auto *token : { "0", "-1", "*", "*32767", "32767" }) {
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = token;
            CHECK(read_town(data) == PARSE_ERROR_NONE);
        }
        for (const auto *token : { "", "1x", "+1", "*-1", "2147483648", "0:1", "0/1", "0\\1" }) {
            auto data = make_town_map();
            data["featureRules"][0]["when"] = "0"; // Inactive rules must still be validated.
            data["featureRules"][0]["definition"][field] = token;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        }
    }
    for (const auto *field : { "monster", "object", "artifact" }) {
        auto data = make_town_map();
        for (const auto *token : { "32768", "*32768", "-32769" }) {
            data["featureRules"][0]["definition"][field] = token;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        }
    }
    auto data = make_town_map();
    data["featureRules"][0]["definition"]["monster"] = "c32767";
    data["featureRules"][0]["definition"]["object"] = "!";
    data["featureRules"][0]["definition"]["artifact"] = "!";
    data["featureRules"][0]["definition"]["ego"] = "-2147483648";
    CHECK(read_town(data) == PARSE_ERROR_NONE);
    for (const auto *token : { "c", "c-1", "c32768", "-32768", "!" }) {
        data["featureRules"][0]["definition"]["monster"] = token;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("TownMapReader rejects out of range JSON integers before conversion")
{
    for (const auto *field : { "caveInfo", "special" }) {
        for (const auto &value : { nlohmann::json(0.0), nlohmann::json(std::numeric_limits<uint64_t>::max()), nlohmann::json("0") }) {
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        }
    }
    auto data = make_town_map();
    data["featureRules"][0]["definition"]["caveInfo"] = std::numeric_limits<int>::max();
    for (const int value : { -32768, 32767 }) {
        data["featureRules"][0]["definition"]["special"] = value;
        CHECK(read_town(data) == PARSE_ERROR_NONE);
    }
    data["featureRules"][0]["definition"]["special"] = 32768;
    CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
    data = make_town_map();
    data["featureRules"][0]["definition"]["caveInfo"] = -1;
    CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
    for (const auto *coordinate : { "x", "y" }) {
        for (const auto &value : { nlohmann::json(-1), nlohmann::json(3), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
            data = make_town_map();
            data["startingPositions"][0][coordinate] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        }
    }
}

TEST_CASE("TownMapReader validates building commands and numeric fields")
{
    auto data = make_town_map();
    auto &rule = data["buildingRules"][0];
    for (const auto &index : { nlohmann::json(-1), nlohmann::json(MAX_BUILDINGS), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        rule["index"] = index;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    rule["index"] = MAX_BUILDINGS - 1;
    rule["command"] = "A";
    rule["fields"] = { "7", "Action", "0", "-1", "a", "0", "0" };
    CHECK(read_town(data) == PARSE_ERROR_NONE);
    for (const auto *index : { "-1", "8", "2147483648" }) {
        rule["fields"][0] = index;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    rule["fields"][0] = "0";
    for (const int index : { 2, 3, 5, 6 }) {
        rule["fields"][index] = "2147483648";
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        rule["fields"][index] = "0";
    }
    for (const auto &[command, count] : { std::pair{ "C", static_cast<int>(PLAYER_CLASS_TYPE_MAX) }, std::pair{ "M", static_cast<int>(MAX_MAGIC) }, std::pair{ "R", MAX_RACES } }) {
        rule["command"] = command;
        rule["fields"] = std::vector<std::string>(count, "0");
        CHECK(read_town(data) == PARSE_ERROR_NONE);
        rule["fields"].push_back("0");
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        rule["fields"] = nlohmann::json::array();
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("TownMapReader rejects legacy delimiters in building text")
{
    for (const auto *text : { "A:B", "A/B", "A\\B", "A\nB", "A\rB" }) {
        auto data = make_town_map();
        data["buildingRules"][0]["fields"][0] = text;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    auto data = make_town_map();
    data["buildingRules"][0]["fields"] = { "Building", "Owner" };
    CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    data = make_town_map();
    data["buildingRules"][0]["locale"] = "other";
    CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    data["buildingRules"][0]["locale"] = "en";
    data["buildingRules"][0]["command"] = "Unknown";
    CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
}

TEST_CASE("TownMapReader validates rectangular printable maps and all start bounds")
{
    for (const auto &rows : { nlohmann::json{ "###", "##" }, nlohmann::json{ "" }, nlohmann::json{ "####" }, nlohmann::json{ "###", "###", "###", "###" }, nlohmann::json{ "##\t" }, nlohmann::json{ "町" }, nlohmann::json{ 3 } }) {
        auto data = make_town_map();
        data["mapVariants"][0]["when"] = "0";
        data["mapVariants"][0]["rows"] = rows;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    auto data = make_town_map();
    data["mapVariants"].push_back({ { "when", "0" }, { "rows", { "#" } } });
    CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    data["startingPositions"][0]["x"] = 0;
    data["startingPositions"][0]["y"] = 0;
    CHECK(read_town(data) == PARSE_ERROR_NONE);
    TownMapDefinition definition;
    CHECK(TownMapReader(data).read(definition, 0, 3) == PARSE_ERROR_INVALID_VALUE);
    CHECK(TownMapReader(data).read(definition, 3, 0) == PARSE_ERROR_INVALID_VALUE);
}

TEST_CASE("TownMapReader building-only mode skips map and start contents, not root requirements")
{
    auto data = make_town_map();
    data["mapVariants"] = { nullptr };
    data["startingPositions"] = { false };
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true) == PARSE_ERROR_NONE);
    CHECK(definition.features.size() == 1);
    CHECK(definition.buildings.size() == 1);
    CHECK(definition.maps.empty());
    CHECK(definition.starts.empty());
    data["featureRules"][0]["symbol"] = ":";
    CHECK(read_town(data, true) == PARSE_ERROR_INVALID_VALUE);
    data = make_town_map();
    data["buildingRules"][0]["command"] = "?";
    CHECK(read_town(data, true) == PARSE_ERROR_INVALID_VALUE);
    data = make_town_map();
    data["mapVariants"] = nlohmann::json::array();
    CHECK(read_town(data, true) == PARSE_ERROR_INVALID_VALUE);
    data = make_town_map();
    data.erase("startingPositions");
    CHECK(read_town(data, true) == PARSE_ERROR_INVALID_TYPE);
}
