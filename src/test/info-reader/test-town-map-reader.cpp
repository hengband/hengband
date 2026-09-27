#include "info-reader/town-map-reader.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/building-type-definition.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace {
std::string hex_bytes(std::string_view value)
{
    if (value.empty()) {
        return "(empty)";
    }
    constexpr std::string_view digits = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char byte : value) {
        if (!result.empty()) {
            result += ' ';
        }
        result += digits[byte >> 4];
        result += digits[byte & 0x0f];
    }
    return result;
}

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

TEST_CASE("TownMapReader byte diagnostics preserve control and UTF-8 bytes")
{
    CHECK(hex_bytes("") == "(empty)");
    CHECK(hex_bytes(std::string_view("A\0", 2)) == "41 00");
    CHECK(hex_bytes("\t\x1f\x7f") == "09 1F 7F");
    CHECK(hex_bytes("\xE7\x94\xBA") == "E7 94 BA");
}

TEST_CASE("TownMapReader retains ordered definitions, conditions and UTF-8 text")
{
    const std::string utf8_name = "\xE7\x94\xBA\xE3\x81\xAE\xE5\xBB\xBA\xE7\x89\xA9";
    REQUIRE(utf8_name.size() == 12);
    auto data = make_town_map();
    for (const auto *field : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        data[field][0]["when"] = "[EQU $TOWN 1]";
        data[field].push_back(data[field][0]);
        data[field][1].erase("when");
    }
    data["buildingRules"][0]["locale"] = "ja";
    data["buildingRules"][0]["fields"][0] = utf8_name;
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
    CHECK(definition.buildings[0].fields[0] == utf8_name);
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
    for (const int value : { -32769, 32768 }) {
        CAPTURE(value);
        data["featureRules"][0]["definition"]["special"] = value;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
    }
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
    for (const auto &rows : { nlohmann::json{ "###", "##" }, nlohmann::json{ "" }, nlohmann::json{ "####" }, nlohmann::json{ "###", "###", "###", "###" }, nlohmann::json{ "##\t" }, nlohmann::json{ "\xE7\x94\xBA" }, nlohmann::json{ 3 } }) {
        CAPTURE(rows.dump(-1, ' ', true));
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

TEST_CASE("TownMapReader accepts unsigned integers produced by JSON parsing")
{
    auto input = make_town_map();
    input["featureRules"][0]["definition"]["caveInfo"] = std::numeric_limits<int>::max();
    input["featureRules"][0]["definition"]["special"] = 32767;
    input["buildingRules"][0]["index"] = MAX_BUILDINGS - 1;
    input["startingPositions"][0]["x"] = 2;
    input["startingPositions"][0]["y"] = 2;
    auto data = nlohmann::json::parse(input.dump());
    REQUIRE(data["version"].is_number_unsigned());
    REQUIRE(data["featureRules"][0]["definition"]["caveInfo"].is_number_unsigned());
    REQUIRE(data["featureRules"][0]["definition"]["special"].is_number_unsigned());
    REQUIRE(data["buildingRules"][0]["index"].is_number_unsigned());
    REQUIRE(data["startingPositions"][0]["x"].is_number_unsigned());
    REQUIRE(data["startingPositions"][0]["y"].is_number_unsigned());
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    CHECK(definition.features[0].cave_info == std::numeric_limits<int>::max());
    CHECK(definition.features[0].special == 32767);
    CHECK(definition.buildings[0].index == MAX_BUILDINGS - 1);
    CHECK(definition.starts[0].x == 2);
    CHECK(definition.starts[0].y == 2);

    input["featureRules"][0]["definition"]["caveInfo"] = 0;
    input["featureRules"][0]["definition"]["special"] = 0;
    input["buildingRules"][0]["index"] = 0;
    input["startingPositions"][0]["x"] = 0;
    input["startingPositions"][0]["y"] = 0;
    data = nlohmann::json::parse(input.dump());
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    CHECK(definition.features[0].cave_info == 0);
    CHECK(definition.features[0].special == 0);
    CHECK(definition.buildings[0].index == 0);
    CHECK(definition.starts[0].x == 0);
    CHECK(definition.starts[0].y == 0);
}

TEST_CASE("TownMapReader preserves building action field error categories")
{
    for (const auto &value : { nlohmann::json(0), nlohmann::json(nullptr), nlohmann::json(false), nlohmann::json::array() }) {
        CAPTURE(value);
        auto data = make_town_map();
        auto &rule = data["buildingRules"][0];
        rule["command"] = "A";
        rule["fields"] = { "0", "Action", "0", "0", "a", "0", "0" };
        rule["fields"][0] = value;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        rule["fields"][0] = "0";
        for (const int index : { 1, 2, 3, 4, 5, 6 }) {
            CAPTURE(index);
            rule["fields"][index] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
            rule["fields"][index] = "0";
        }
    }
}

TEST_CASE("TownMapReader leaves all output intact on failure at every validation stage")
{
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3) == PARSE_ERROR_NONE);
    for (const auto *stage : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        CAPTURE(std::string(stage));
        auto data = original;
        data["featureRules"][0]["symbol"] = ".";
        data["buildingRules"][0]["fields"][0] = "Changed";
        data["mapVariants"][0]["rows"][0] = "...";
        data["startingPositions"][0]["x"] = 2;
        data[stage].push_back(nullptr);
        CHECK(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_INVALID_TYPE);
        REQUIRE(definition.features.size() == 1);
        REQUIRE(definition.buildings.size() == 1);
        REQUIRE(definition.maps.size() == 1);
        REQUIRE(definition.starts.size() == 1);
        CHECK(definition.features[0].symbol == '#');
        CHECK(definition.buildings[0].fields[0] == "Building");
        CHECK(definition.maps[0].rows[0] == "###");
        CHECK(definition.starts[0].x == 1);
    }
}

TEST_CASE("TownMapReader building-only reread preserves output on failure and replaces it on success")
{
    auto data = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    REQUIRE_FALSE(definition.maps.empty());
    REQUIRE_FALSE(definition.starts.empty());
    data["featureRules"][0]["symbol"] = ".";
    data["buildingRules"][0]["fields"][0] = "Changed";
    for (const auto &invalid : { nlohmann::json(nullptr), nlohmann::json{ { "index", -1 }, { "locale", "en" }, { "command", "Z" }, { "fields", nlohmann::json::array() } } }) {
        const auto expected = invalid.is_null() ? PARSE_ERROR_INVALID_TYPE : PARSE_ERROR_INVALID_VALUE;
        CAPTURE(invalid);
        auto bad = data;
        bad["buildingRules"].push_back(invalid);
        CHECK(TownMapReader(bad).read(definition, 0, 0, true) == expected);
        REQUIRE(definition.features.size() == 1);
        REQUIRE(definition.buildings.size() == 1);
        REQUIRE(definition.maps.size() == 1);
        REQUIRE(definition.starts.size() == 1);
        CHECK(definition.features[0].symbol == '#');
        CHECK(definition.buildings[0].fields[0] == "Building");
        CHECK(definition.maps[0].rows[0] == "###");
        CHECK(definition.starts[0].x == 1);
    }
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true) == PARSE_ERROR_NONE);
    REQUIRE(definition.features.size() == 1);
    REQUIRE(definition.buildings.size() == 1);
    CHECK(definition.features[0].symbol == '.');
    CHECK(definition.buildings[0].fields[0] == "Changed");
    CHECK(definition.maps.empty());
    CHECK(definition.starts.empty());
}

TEST_CASE("TownMapReader validates feature symbols and token types")
{
    for (const auto *symbol : { "", "##", ":", "/", "\\", "\t", "\x1f", "\x7f", "\xE7\x94\xBA" }) {
        CAPTURE(hex_bytes(symbol));
        auto data = make_town_map();
        data["featureRules"][0]["symbol"] = symbol;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    for (const auto *symbol : { " ", "~" }) {
        CAPTURE(hex_bytes(symbol));
        auto data = make_town_map();
        data["featureRules"][0]["symbol"] = symbol;
        CHECK(read_town(data) == PARSE_ERROR_NONE);
    }
    for (const auto *field : { "terrain", "monster", "object", "ego", "artifact", "trap" }) {
        CAPTURE(std::string(field));
        auto data = make_town_map();
        data["featureRules"][0]["definition"][field] = nullptr;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        data["featureRules"][0]["definition"][field] = "bad/field";
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        data["featureRules"][0]["definition"][field] = 0;
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        data["featureRules"][0]["definition"].erase(field);
        CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
    }
}

TEST_CASE("TownMapReader rejects parsed unsigned integers immediately above each upper bound")
{
    struct Boundary {
        const char *section;
        const char *field;
        uint64_t value;
        parse_error_type error;
    };
    for (const auto &boundary : {
             Boundary{ "featureRules", "special", 32768, PARSE_ERROR_INVALID_TYPE },
             Boundary{ "featureRules", "caveInfo", static_cast<uint64_t>(std::numeric_limits<int>::max()) + 1, PARSE_ERROR_INVALID_TYPE },
             Boundary{ "buildingRules", "index", static_cast<uint64_t>(MAX_BUILDINGS), PARSE_ERROR_INVALID_VALUE },
             Boundary{ "startingPositions", "x", 3, PARSE_ERROR_INVALID_VALUE },
             Boundary{ "startingPositions", "y", 3, PARSE_ERROR_INVALID_VALUE },
         }) {
        CAPTURE(std::string(boundary.section));
        CAPTURE(std::string(boundary.field));
        CAPTURE(boundary.value);
        auto data = make_town_map();
        auto &record = data[boundary.section][0];
        auto &value = std::string_view(boundary.section) == "featureRules" ? record["definition"][boundary.field] : record[boundary.field];
        value = nlohmann::json::parse(std::to_string(boundary.value));
        REQUIRE(value.is_number_unsigned());
        CHECK(read_town(data) == boundary.error);
    }
}

TEST_CASE("TownMapReader rejects wrong action field counts and malformed numeric fields")
{
    for (const size_t count : { 0U, 6U, 8U }) {
        CAPTURE(count);
        auto data = make_town_map();
        data["buildingRules"][0]["command"] = "A";
        data["buildingRules"][0]["fields"] = std::vector<std::string>(count, "0");
        CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
    }
    for (const int index : { 0, 2, 3, 5, 6 }) {
        CAPTURE(index);
        for (const auto *value : { "", "1x", " 1" }) {
            CAPTURE(hex_bytes(value));
            auto data = make_town_map();
            data["buildingRules"][0]["command"] = "A";
            data["buildingRules"][0]["fields"] = { "0", "Action", "0", "0", "a", "0", "0" };
            data["buildingRules"][0]["fields"][index] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        }
    }
    for (const auto *command : { "N", "C", "M", "R", "Z" }) {
        CAPTURE(std::string(command));
        for (const auto &value : { nlohmann::json(0), nlohmann::json(nullptr) }) {
            CAPTURE(value);
            auto data = make_town_map();
            auto &rule = data["buildingRules"][0];
            rule["command"] = command;
            rule["fields"] = std::vector<std::string>(std::string_view(command) == "N" ? 3 : 1, "0");
            rule["fields"][0] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_TYPE);
        }
    }
}

TEST_CASE("TownMapReader preserves output on root errors and value errors at every stage")
{
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3) == PARSE_ERROR_NONE);
    for (const auto *failure : { "version", "emptyMaps", "missingFeatures", "symbol", "locale", "row", "coordinate" }) {
        CAPTURE(std::string(failure));
        auto data = original;
        data["featureRules"][0]["symbol"] = ".";
        data["buildingRules"][0]["fields"][0] = "Changed";
        data["mapVariants"][0]["rows"][0] = "...";
        data["startingPositions"][0]["x"] = 2;
        parse_error_type expected = PARSE_ERROR_INVALID_VALUE;
        const std::string_view kind = failure;
        if (kind == "version") {
            data["version"] = 1;
            expected = PARSE_ERROR_INVALID_TYPE;
        } else if (kind == "emptyMaps") {
            data["mapVariants"] = nlohmann::json::array();
        } else if (kind == "missingFeatures") {
            data.erase("featureRules");
            expected = PARSE_ERROR_INVALID_TYPE;
        } else if (kind == "symbol") {
            data["featureRules"][0]["symbol"] = ":";
        } else if (kind == "locale") {
            data["buildingRules"][0]["locale"] = "other";
        } else if (kind == "row") {
            data["mapVariants"][0]["rows"][0] = "##";
        } else {
            data["startingPositions"][0]["x"] = 3;
        }
        CHECK(TownMapReader(data).read(definition, 3, 3) == expected);
        REQUIRE(definition.features.size() == 1);
        REQUIRE(definition.buildings.size() == 1);
        REQUIRE(definition.maps.size() == 1);
        REQUIRE(definition.starts.size() == 1);
        CHECK(definition.features[0].symbol == '#');
        CHECK(definition.buildings[0].fields[0] == "Building");
        CHECK(definition.maps[0].rows[0] == "###");
        CHECK(definition.starts[0].x == 1);
    }
}

TEST_CASE("TownMapReader rejects nonprintable terrain and trap tokens")
{
    for (const auto *field : { "terrain", "trap" }) {
        CAPTURE(std::string(field));
        for (const auto &value : { std::string("WALL\t"), std::string("\x1f"), std::string("\x7f"), std::string("\xE7\x94\xBA"), std::string("WALL\0", 5) }) {
            CAPTURE(hex_bytes(value));
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = value;
            CHECK(read_town(data) == PARSE_ERROR_INVALID_VALUE);
        }
    }
}
