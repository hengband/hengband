#include "artifact/fixed-art-types.h"
#include "info-reader/general-parser.h"
#include "info-reader/random-grid-effect-types.h"
#include "info-reader/town-map-reader.h"
#include "locale/character-encoding.h"
#include "object-enchant/trg-types.h"
#include "object/tval-types.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/building-type-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/terrain-list-test-access.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace test {
class QuestFeatureTestAccess {
public:
    QuestFeatureTestAccess()
    {
        QuestList::get_instance().quests.swap(previous_quests);
        QuestList::get_instance().quests.try_emplace(QuestId::THIEF);
    }

    ~QuestFeatureTestAccess()
    {
        QuestList::get_instance().quests.swap(previous_quests);
    }

    QuestFeatureTestAccess(const QuestFeatureTestAccess &) = delete;
    QuestFeatureTestAccess &operator=(const QuestFeatureTestAccess &) = delete;

    // 読込テストではアーティファクト予約を行わず、既に確定した報酬だけを用意する。
    static void set_resolved_reward(FixedArtifactId id)
    {
        QuestList::get_instance().get_quest(QuestId::THIEF).reward_fa_id = id;
    }

private:
    std::map<QuestId, QuestType> previous_quests;
};
}

namespace {
class ScopedTownBuildings {
public:
    ScopedTownBuildings()
        : saved(buildings)
    {
        for (auto &building : buildings) {
            building = {};
            building.member_class.assign(PLAYER_CLASS_TYPE_MAX, -7);
            building.member_race.assign(MAX_RACES, -7);
            building.member_realm.assign(MAX_MAGIC + 1, -7);
        }
    }

    ~ScopedTownBuildings()
    {
        buildings = std::move(this->saved);
    }

    ScopedTownBuildings(const ScopedTownBuildings &) = delete;
    ScopedTownBuildings &operator=(const ScopedTownBuildings &) = delete;

private:
    std::array<building_type, MAX_BUILDINGS> saved;
};

void check_building(const building_type &actual, const building_type &expected)
{
    CHECK(std::string(actual.name) == expected.name);
    CHECK(std::string(actual.owner_name) == expected.owner_name);
    CHECK(std::string(actual.owner_race) == expected.owner_race);
    for (size_t i = 0; i < 8; ++i) {
        CHECK(std::string(actual.act_names[i]) == expected.act_names[i]);
        CHECK(actual.member_costs[i] == expected.member_costs[i]);
        CHECK(actual.other_costs[i] == expected.other_costs[i]);
        CHECK(actual.letters[i] == expected.letters[i]);
        CHECK(actual.actions[i] == expected.actions[i]);
        CHECK(actual.action_restr[i] == expected.action_restr[i]);
    }
    CHECK(actual.member_class == expected.member_class);
    CHECK(actual.member_race == expected.member_race);
    CHECK(actual.member_realm == expected.member_realm);
}

class TownFeatureStateGuard {
public:
    TownFeatureStateGuard()
        : saved_init_flags(init_flags)
        , terrain_count(TerrainList::get_instance().size())
        , none_tag(TerrainTag::NONE, 0)
    {
        std::copy(std::begin(letter), std::end(letter), saved_letters.begin());
        init_flags = static_cast<init_flags_type>(0);
        auto &terrains = TerrainList::get_instance();
        terrains.resize(terrain_count + 2);
        terrains.get_terrain(static_cast<short>(terrain_count)).tag = "TOWN_FEATURE_TEST_FLOOR";
        terrains.get_terrain(static_cast<short>(terrain_count + 1)).tag = "TOWN_FEATURE_TEST_TRAP";
    }

    ~TownFeatureStateGuard()
    {
        std::copy(saved_letters.begin(), saved_letters.end(), std::begin(letter));
        init_flags = saved_init_flags;
        TerrainList::get_instance().resize(terrain_count);
    }

    TownFeatureStateGuard(const TownFeatureStateGuard &) = delete;
    TownFeatureStateGuard &operator=(const TownFeatureStateGuard &) = delete;

    short floor_id() const
    {
        return static_cast<short>(terrain_count);
    }
    short trap_id() const
    {
        return static_cast<short>(terrain_count + 1);
    }

private:
    init_flags_type saved_init_flags;
    size_t terrain_count;
    std::array<dungeon_grid, 255> saved_letters;
    test::TerrainListTestAccess none_tag;
    test::ScopedReaderState reader_state;
};

void check_same_grid(const dungeon_grid &actual, const dungeon_grid &expected)
{
    CHECK(actual.feature == expected.feature);
    CHECK(actual.monster == expected.monster);
    CHECK(actual.object == expected.object);
    CHECK(actual.ego == expected.ego);
    CHECK(actual.artifact == expected.artifact);
    CHECK(actual.trap == expected.trap);
    CHECK(actual.cave_info == expected.cave_info);
    CHECK(actual.special == expected.special);
    CHECK(actual.random == expected.random);
}

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

TownMapDefinition read_reward_town_map(FloorType &floor, ArtifactDefinition artifact)
{
    ArtifactList::get_instance().emplace(FixedArtifactId::GALADRIEL_PHIAL, std::move(artifact));
    floor.quest_number = QuestId::THIEF;
    auto data = make_town_map();
    auto &fields = data["featureRules"][0]["definition"];
    fields["terrain"] = "TOWN_FEATURE_TEST_FLOOR";
    fields["trap"] = "TOWN_FEATURE_TEST_TRAP";
    fields["object"] = "!";
    fields["artifact"] = "!";
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    return definition;
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
    CHECK(definition.features[0].cell.grid.monster == 0);
    CHECK(definition.features[0].cell.grid.object == 0);
    CHECK(enum2i(definition.features[0].cell.grid.ego) == 0);
    CHECK(enum2i(definition.features[0].cell.grid.artifact) == 0);
    CHECK(definition.features[0].trap == "NONE");
    CHECK(definition.features[0].cell.grid.cave_info == 0);
    CHECK(definition.features[0].cell.grid.special == 0);
    CHECK(definition.features[0].condition == "[EQU $TOWN 1]");
    CHECK_FALSE(definition.features[1].condition.has_value());
    REQUIRE(definition.buildings.size() == 2);
    CHECK(definition.buildings[0].index == 1);
    CHECK_FALSE(definition.buildings[0].english);
    CHECK(definition.buildings[1].english);
    REQUIRE(std::holds_alternative<TownMapBuildingNames>(definition.buildings[0].directive));
    CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == utf8_name);
    CHECK(definition.buildings[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.maps.size() == 2);
    CHECK(definition.maps[0].rows == std::vector<std::string>{ "###", "#.#", "###" });
    CHECK(definition.maps[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.starts.size() == 2);
    CHECK(definition.starts[0].y == 1);
    CHECK(definition.starts[0].x == 1);
    CHECK(definition.starts[0].condition == "[EQU $TOWN 1]");
}

TEST_CASE("TownMapReader keeps percent and M as map payload and realm membership commands")
{
    const test::ScopedReaderState reader_state;
    auto data = make_town_map();
    data["featureRules"][0]["symbol"] = "%";
    data["featureRules"].push_back(data["featureRules"][0]);
    data["featureRules"][1]["symbol"] = "M";
    data["mapVariants"][0]["rows"] = { "%M%", "M%M", "%M%" };
    data["buildingRules"][0]["command"] = "M";
    auto realms = std::vector<std::string>(MAX_MAGIC, "0");
    realms.back() = "1";
    data["buildingRules"][0]["fields"] = realms;
    for (const auto *field : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        data[field][0]["when"] = "[EQU $TOWN 1]";
    }

    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    REQUIRE(definition.features.size() == 2);
    CHECK(definition.features[0].symbol == '%');
    CHECK(definition.features[1].symbol == 'M');
    CHECK(definition.features[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.buildings.size() == 1);
    const auto &membership = std::get<TownMapBuildingMembership>(definition.buildings[0].directive);
    CHECK(membership.kind == TownMapBuildingMembershipKind::REALM);
    REQUIRE(membership.values.size() == MAX_MAGIC);
    CHECK(membership.values.front() == 0);
    CHECK(membership.values.back() == 1);
    CHECK(definition.buildings[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.maps.size() == 1);
    CHECK(definition.maps[0].rows == std::vector<std::string>{ "%M%", "M%M", "%M%" });
    CHECK(definition.maps[0].condition == "[EQU $TOWN 1]");
    REQUIRE(definition.starts.size() == 1);
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

TEST_CASE("TownMapReader preserves JSON integer error categories and output on failure")
{
    struct Field {
        const char *section;
        const char *name;
        parse_error_type range_error;
    };
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3) == PARSE_ERROR_NONE);
    for (const auto &field : {
             Field{ "featureRules", "caveInfo", PARSE_ERROR_INVALID_TYPE },
             Field{ "featureRules", "special", PARSE_ERROR_INVALID_TYPE },
             Field{ "buildingRules", "index", PARSE_ERROR_INVALID_VALUE },
             Field{ "startingPositions", "x", PARSE_ERROR_INVALID_VALUE },
             Field{ "startingPositions", "y", PARSE_ERROR_INVALID_VALUE },
         }) {
        CAPTURE(std::string(field.section));
        CAPTURE(std::string(field.name));
        for (const auto &value : {
                 nlohmann::json(nullptr),
                 nlohmann::json(false),
                 nlohmann::json(1.0),
                 nlohmann::json("1"),
                 nlohmann::json::array(),
                 nlohmann::json::object(),
                 nlohmann::json(std::numeric_limits<int64_t>::min()),
                 nlohmann::json(std::numeric_limits<int64_t>::max()),
                 nlohmann::json(std::numeric_limits<uint64_t>::max()),
             }) {
            CAPTURE(value);
            auto data = original;
            data["featureRules"][0]["symbol"] = ".";
            data["buildingRules"][0]["fields"][0] = "Changed";
            data["mapVariants"][0]["rows"][0] = "...";
            data["startingPositions"][0]["x"] = 2;
            auto &record = data[field.section][0];
            auto &fields = std::string_view(field.section) == "featureRules" ? record["definition"] : record;
            fields[field.name] = value;
            const auto expected = value.is_number_integer() ? field.range_error : PARSE_ERROR_INVALID_TYPE;
            for (const bool missing : { false, true }) {
                CAPTURE(missing);
                if (missing) {
                    fields.erase(field.name);
                }
                CHECK(TownMapReader(data).read(definition, 3, 3) == (missing ? PARSE_ERROR_INVALID_TYPE : expected));
                REQUIRE(definition.features.size() == 1);
                REQUIRE(definition.buildings.size() == 1);
                REQUIRE(definition.maps.size() == 1);
                REQUIRE(definition.starts.size() == 1);
                CHECK(definition.features[0].symbol == '#');
                CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
                CHECK(definition.maps[0].rows[0] == "###");
                CHECK(definition.starts[0].x == 1);
            }
        }
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
    CHECK(definition.features[0].cell.grid.cave_info == std::numeric_limits<int>::max());
    CHECK(definition.features[0].cell.grid.special == 32767);
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
    CHECK(definition.features[0].cell.grid.cave_info == 0);
    CHECK(definition.features[0].cell.grid.special == 0);
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
        CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
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
        CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
        CHECK(definition.maps[0].rows[0] == "###");
        CHECK(definition.starts[0].x == 1);
    }
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true) == PARSE_ERROR_NONE);
    REQUIRE(definition.features.size() == 1);
    REQUIRE(definition.buildings.size() == 1);
    CHECK(definition.features[0].symbol == '.');
    CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Changed");
    CHECK(definition.maps.empty());
    CHECK(definition.starts.empty());
}

TEST_CASE("TownMapReader rejects non ASCII feature and row bytes without publishing output")
{
    for (const auto byte : { 0x00, 0x1f, 0x7f, 0x80, 0xfe, 0xff }) {
        for (const auto feature : { false, true }) {
            CAPTURE(byte);
            CAPTURE(feature);
            auto data = make_town_map();
            if (feature) {
                data["featureRules"][0]["symbol"] = std::string(1, static_cast<char>(byte));
            } else {
                data["mapVariants"][0]["rows"][0] = std::string(3, static_cast<char>(byte));
            }
            TownMapDefinition definition;
            definition.features.emplace_back().symbol = '?';
            CHECK(TownMapReader(data).read(definition, 10, 10) == PARSE_ERROR_INVALID_VALUE);
            REQUIRE(definition.features.size() == 1);
            CHECK(definition.features.front().symbol == '?');
            CHECK(definition.maps.empty());
        }
    }
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
        CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
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

TEST_CASE("TownMapReader typed building application matches legacy N A C R M Z in both locales")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    for (const auto *locale : { "en", "ja" }) {
        CAPTURE(std::string(locale));
        for (const auto &[command, fields] : {
                 std::pair{ "N", std::vector<std::string>{ "Building name that is longer than nineteen bytes", "Owner", "Race" } },
                 std::pair{ "N", std::vector<std::string>{ "", "", "" } },
                 std::pair{ "N", std::vector<std::string>{ "\xE7\x94\xBA\xE3\x81\xAE\xE5\xBB\xBA\xE7\x89\xA9", "Owner", "Race" } },
                 std::pair{ "N", std::vector<std::string>{ "\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA\xE7\x94\xBA", "Owner", "Race" } },
                 std::pair{ "A", std::vector<std::string>{ "7", "Action name that is longer than twenty nine bytes", "2147483647", "-2147483648", "abc", "65535", "-32769" } },
                 std::pair{ "A", std::vector<std::string>{ "0", "", "0", "0", "", "0", "0" } },
                 std::pair{ "C", std::vector<std::string>{ "2", "0" } },
                 std::pair{ "R", std::vector<std::string>{ "0", "65535" } },
                 std::pair{ "M", std::vector<std::string>{ "3", "-1" } },
                 std::pair{ "Z", std::vector<std::string>{ "Ignored", "\xE7\x94\xBA" } },
                 std::pair{ "Z", std::vector<std::string>{} },
             }) {
            CAPTURE(std::string(command));
            auto data = make_town_map();
            data["buildingRules"][0]["locale"] = locale;
            data["buildingRules"][0]["command"] = command;
            data["buildingRules"][0]["fields"] = fields;
            TownMapDefinition definition;
            REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
            REQUIRE(definition.buildings.size() == 1);
            std::string line = std::string(locale) == "en" ? "B:$1:" : "B:1:";
            line += command;
            for (const auto &field : fields) {
                line += ":" + field;
            }
            const auto encoded = utf8_to_sys(line);
            REQUIRE(encoded.has_value());
            buildings[1] = initial;
            REQUIRE(parse_line_building(*encoded) == PARSE_ERROR_NONE);
            const auto expected = buildings[1];
            buildings[1] = initial;
            REQUIRE(apply_town_building_rule(definition.buildings[0]) == PARSE_ERROR_NONE);
            check_building(buildings[1], expected);
            check_building(buildings[2], initial);
        }
    }
}

TEST_CASE("TownMapReader typed building memberships default omitted entries and preserve realm zero")
{
    const ScopedTownBuildings state;
    for (const auto *command : { "C", "R", "M" }) {
        CAPTURE(std::string(command));
        auto data = make_town_map();
        data["buildingRules"][0]["locale"] = _("ja", "en");
        data["buildingRules"][0]["command"] = command;
        data["buildingRules"][0]["fields"] = { "2" };
        TownMapDefinition definition;
        REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
        REQUIRE(apply_town_building_rule(definition.buildings[0]) == PARSE_ERROR_NONE);
        const auto is_realm = std::string_view(command) == "M";
        const auto &members = std::string_view(command) == "C" ? buildings[1].member_class : (is_realm ? buildings[1].member_realm : buildings[1].member_race);
        const auto offset = is_realm ? 1U : 0U;
        CHECK(members[offset] == 2);
        for (size_t i = offset + 1; i < members.size(); ++i) {
            CHECK(members[i] == 1);
        }
        CHECK(buildings[1].member_realm[0] == -7);
    }
}

TEST_CASE("TownMapReader typed building application validates conversion before locale filtering")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    for (const auto *locale : { "en", "ja" }) {
        for (const auto *command : { "N", "A", "Z" }) {
            auto data = make_town_map();
            auto &rule = data["buildingRules"][0];
            rule["locale"] = locale;
            rule["command"] = command;
            const std::string invalid_utf8 = "\xff";
            if (std::string_view(command) == "N") {
                rule["fields"] = { "Name", "Owner", invalid_utf8 };
            } else if (std::string_view(command) == "A") {
                rule["fields"] = { "0", "Action", "0", "0", invalid_utf8, "0", "0" };
            } else {
                rule["fields"] = { invalid_utf8 };
            }
            TownMapDefinition definition;
            REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
            const auto expected = utf8_to_sys(invalid_utf8) ? PARSE_ERROR_NONE : PARSE_ERROR_INVALID_VALUE;
            CHECK(apply_town_building_rule(definition.buildings[0]) == expected);
            // 英語版は文字コード変換が恒等変換のため、対象言語のN/Aは適用される。
            if (expected != PARSE_ERROR_NONE || std::string_view(locale) != _("ja", "en") || std::string_view(command) == "Z") {
                check_building(buildings[1], initial);
            }
            buildings[1] = initial;
        }
    }
}

TEST_CASE("TownMapReader typed building failures leave the target unchanged")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    auto data = make_town_map();
    data["buildingRules"][0]["locale"] = _("ja", "en");
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    auto rule = definition.buildings[0];
    rule.index = MAX_BUILDINGS;
    CHECK(apply_town_building_rule(rule) == PARSE_ERROR_INVALID_VALUE);
    rule.index = 1;
    rule.directive = TownMapBuildingAction{ 8, "Name", 1, 2, "a", 3, 4 };
    CHECK(apply_town_building_rule(rule) == PARSE_ERROR_INVALID_VALUE);
    rule.directive = TownMapBuildingMembership{ TownMapBuildingMembershipKind::CLASS, {} };
    CHECK(apply_town_building_rule(rule) == PARSE_ERROR_INVALID_VALUE);
    check_building(buildings[1], initial);
    data["buildingRules"].push_back(data["buildingRules"][0]);
    data["buildingRules"][1]["command"] = "A";
    data["buildingRules"][1]["fields"] = { "0", "Action", "0", "0", "a", "overflow", "0" };
    CHECK(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_INVALID_VALUE);
    REQUIRE(definition.buildings.size() == 1);
    CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
    check_building(buildings[1], initial);
}

TEST_CASE("TownMapReader typed building application rejects embedded NUL before any writes")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    const std::string embedded_nul("Name\0suffix", 11);
    for (const auto *locale : { "en", "ja" }) {
        for (const auto *command : { "N", "A", "Z" }) {
            auto data = make_town_map();
            auto &rule = data["buildingRules"][0];
            rule["locale"] = locale;
            rule["command"] = command;
            if (std::string_view(command) == "N") {
                rule["fields"] = { embedded_nul, "Owner", "Race" };
            } else if (std::string_view(command) == "A") {
                rule["fields"] = { "0", embedded_nul, "0", "0", "a", "0", "0" };
            } else {
                rule["fields"] = { embedded_nul };
            }
            TownMapDefinition definition;
            // 区切り文字の検証とは別に、適用側でC文字列にできない文字を拒否する。
            REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
            CHECK(apply_town_building_rule(definition.buildings[0]) == PARSE_ERROR_INVALID_VALUE);
            check_building(buildings[1], initial);
        }
    }
#ifndef JP
    // 旧dispatcherのchar*から作られるviewはNULの位置で切れ、Nの残り2引数を失う。
    const auto legacy_line = std::string("B:$1:N:") + embedded_nul + ":Owner:Race";
    CHECK(parse_line_building(std::string_view(legacy_line.c_str())) == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    check_building(buildings[1], initial);
#endif
}

TEST_CASE("TownMapReader applies typed feature tokens with legacy F semantics")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    for (const auto *token : { "0", "-1", "*", "*32767", "32767" }) {
        auto data = make_town_map();
        auto &fields = data["featureRules"][0]["definition"];
        fields["terrain"] = "TOWN_FEATURE_TEST_FLOOR";
        fields["trap"] = "TOWN_FEATURE_TEST_TRAP";
        fields["caveInfo"] = std::numeric_limits<int>::max();
        fields["special"] = -32768;
        for (const auto *field : { "monster", "object", "ego", "artifact" }) {
            fields[field] = token;
        }
        TownMapDefinition definition;
        REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
        const auto legacy = std::string("F:#:TOWN_FEATURE_TEST_FLOOR:2147483647:") + token + ":" + token + ":" + token + ":" + token + ":TOWN_FEATURE_TEST_TRAP:-32768";
        REQUIRE(parse_line_feature(floor, legacy) == PARSE_ERROR_NONE);
        const auto expected = letter['#'];
        letter['#'] = {};
        REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
        check_same_grid(letter['#'], expected);
        CHECK(letter['#'].feature == state.floor_id());
        CHECK(letter['#'].trap == state.trap_id());
    }
}

TEST_CASE("TownMapReader typed and legacy features reject unsafe bytes before touching letters")
{
    const TownFeatureStateGuard state;
    const FloorType floor;
    for (const auto byte : { 0x00, 0x1f, 0x7f, 0x80, 0xfe, 0xff }) {
        CAPTURE(byte);
        TownMapFeatureRule feature;
        feature.symbol = static_cast<char>(byte);
        feature.terrain = "TOWN_FEATURE_TEST_FLOOR";
        feature.trap = "NONE";
        const std::array<dungeon_grid, 255> original = [] {
            std::array<dungeon_grid, 255> result;
            std::copy(std::begin(letter), std::end(letter), result.begin());
            return result;
        }();
        CHECK(apply_town_map_feature(floor, feature) == PARSE_ERROR_INVALID_VALUE);
        const auto line = "F:" + std::string(1, feature.symbol) + ":TOWN_FEATURE_TEST_FLOOR";
        CHECK(parse_line_feature(floor, line) == PARSE_ERROR_INVALID_VALUE);
        for (size_t i = 0; i < original.size(); ++i) {
            check_same_grid(letter[i], original[i]);
        }
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(apply_town_map_feature(floor, feature) == PARSE_ERROR_NONE);
        CHECK(parse_line_feature(floor, line) == PARSE_ERROR_NONE);
        init_flags = static_cast<init_flags_type>(0);
    }
}

TEST_CASE("TownMapReader legacy feature rejects empty and multi character symbols without writes")
{
    const TownFeatureStateGuard state;
    const FloorType floor;
    for (const auto *symbol : { "", "AB" }) {
        CAPTURE(symbol);
        letter['A'].special = 73;
        letter['B'].special = 91;
        const auto first = letter['A'];
        const auto second = letter['B'];
        const auto line = std::string("F:") + symbol + ":TOWN_FEATURE_TEST_FLOOR";
        CHECK(parse_line_feature(floor, line) == PARSE_ERROR_INVALID_VALUE);
        check_same_grid(letter['A'], first);
        check_same_grid(letter['B'], second);
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(parse_line_feature(floor, line) == PARSE_ERROR_NONE);
        check_same_grid(letter['A'], first);
        check_same_grid(letter['B'], second);
        init_flags = static_cast<init_flags_type>(0);
    }
}

TEST_CASE("TownMapReader typed and legacy features retain printable ASCII boundaries")
{
    const TownFeatureStateGuard state;
    const FloorType floor;
    for (const auto symbol : { ' ', '~' }) {
        CAPTURE(symbol);
        TownMapFeatureRule feature;
        feature.symbol = symbol;
        feature.terrain = "TOWN_FEATURE_TEST_FLOOR";
        feature.trap = "TOWN_FEATURE_TEST_TRAP";
        REQUIRE(apply_town_map_feature(floor, feature) == PARSE_ERROR_NONE);
        CHECK(letter[static_cast<unsigned char>(symbol)].feature == state.floor_id());
        CHECK(letter[static_cast<unsigned char>(symbol)].trap == state.trap_id());
        letter[static_cast<unsigned char>(symbol)].feature = 0;
        REQUIRE(parse_line_feature(floor, "F:" + std::string(1, symbol) + ":TOWN_FEATURE_TEST_FLOOR") == PARSE_ERROR_NONE);
        CHECK(letter[static_cast<unsigned char>(symbol)].feature == state.floor_id());
    }
}

TEST_CASE("TownMapReader applies random terrain trap clones and reward placeholders")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    auto data = make_town_map();
    auto &fields = data["featureRules"][0]["definition"];
    fields["terrain"] = "*";
    fields["trap"] = "*";
    fields["monster"] = "c32767";
    fields["object"] = "!";
    fields["artifact"] = "!";
    fields["ego"] = "-2147483648";
    fields["special"] = 32767;
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
    REQUIRE(parse_line_feature(floor, "F:#:*:0:c32767:!:-2147483648:!:*:32767") == PARSE_ERROR_NONE);
    const auto expected = letter['#'];
    letter['#'] = {};
    REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
    check_same_grid(letter['#'], expected);
    CHECK(letter['#'].monster == -32767);
    CHECK(letter['#'].random == (RANDOM_FEATURE | RANDOM_TRAP));
    CHECK(definition.features.front().cell.object_is_quest_reward);
    CHECK(definition.features.front().cell.artifact_is_quest_reward);
}

TEST_CASE("TownMapReader defers terrain resolution and publishes feature only on success")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    for (const auto *field : { "terrain", "trap" }) {
        auto data = make_town_map();
        data["featureRules"][0]["definition"]["terrain"] = "TOWN_FEATURE_TEST_FLOOR";
        data["featureRules"][0]["definition"]["trap"] = "TOWN_FEATURE_TEST_TRAP";
        data["featureRules"][0]["definition"][field] = "TOWN_FEATURE_TEST_UNDEFINED";
        data["featureRules"][0]["when"] = "0";
        TownMapDefinition definition;
        REQUIRE(TownMapReader(data).read(definition, 3, 3) == PARSE_ERROR_NONE);
        CHECK(definition.features.front().condition == "0");
        letter['#'].feature = 123;
        letter['#'].monster = 456;
        letter['#'].random = RANDOM_EGO;
        const auto original = letter['#'];
        CHECK(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
        check_same_grid(letter['#'], original);
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
        check_same_grid(letter['#'], original);
        init_flags = static_cast<init_flags_type>(0);
    }
}

TEST_CASE("TownMapReader resolves quest rewards at feature application time")
{
    TownFeatureStateGuard state;
    const test::QuestFeatureTestAccess quest_state;
    const test::ArtifactListTestAccess artifact_state;
    ArtifactDefinition artifact;
    artifact.gen_flags.set(ItemGenerationTraitType::INSTA_ART);
    FloorType floor;
    const auto definition = read_reward_town_map(floor, std::move(artifact));
    REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
    CHECK(letter['#'].object == 0);
    CHECK(letter['#'].artifact == FixedArtifactId::NONE);
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);
    REQUIRE(parse_line_feature(floor, "F:#:TOWN_FEATURE_TEST_FLOOR:0:0:!:0:!:TOWN_FEATURE_TEST_TRAP:0") == PARSE_ERROR_NONE);
    const auto expected = letter['#'];
    letter['#'] = {};
    REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
    check_same_grid(letter['#'], expected);
    CHECK(letter['#'].object == 0); // Instant artifacts do not generate a base item.
    CHECK(letter['#'].artifact == FixedArtifactId::GALADRIEL_PHIAL);
    floor.quest_number = QuestId::NONE;
    REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
    CHECK(letter['#'].artifact == FixedArtifactId::NONE);
}

TEST_CASE("TownMapReader applies resolved ordinary quest reward baseitem and artifact IDs")
{
    const TownFeatureStateGuard state;
    const test::QuestFeatureTestAccess quest_state;
    const test::ArtifactListTestAccess artifact_state;
    auto &items = BaseitemList::get_instance();
    const test::ScopedVectorWrapper items_state(items);
    const BaseitemKey reward_key{ ItemKindType::SWORD, 37 };
    ArtifactDefinition artifact;
    artifact.bi_key = reward_key;
    REQUIRE_FALSE(artifact.is_instant_artifact());
    FloorType floor;
    const auto definition = read_reward_town_map(floor, std::move(artifact));
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);

    items.resize(8);
    BaseitemDefinition original;
    original.name = "Town reward original";
    original.bi_key = reward_key;
    items.replace_baseitem(7, std::move(original));
    REQUIRE(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_NONE);
    CHECK(letter['#'].object == 7);
    CHECK(letter['#'].artifact == FixedArtifactId::GALADRIEL_PHIAL);
    CHECK(letter['#'].feature == state.floor_id());
    CHECK(letter['#'].trap == state.trap_id());
}

TEST_CASE("TownMapReader returns an error and preserves the feature when ordinary reward baseitem lookup fails")
{
    const TownFeatureStateGuard state;
    const test::QuestFeatureTestAccess quest_state;
    const test::ArtifactListTestAccess artifact_state;
    auto &items = BaseitemList::get_instance();
    const test::ScopedVectorWrapper items_state(items);
    ArtifactDefinition artifact;
    artifact.bi_key = { ItemKindType::SWORD, 37 };
    REQUIRE_FALSE(artifact.is_instant_artifact());
    FloorType floor;
    const auto definition = read_reward_town_map(floor, std::move(artifact));
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);
    letter['#'].feature = 123;
    letter['#'].trap = 45;
    letter['#'].monster = 67;
    letter['#'].object = 89;
    letter['#'].ego = static_cast<EgoType>(12);
    letter['#'].artifact = FixedArtifactId::GALADRIEL_PHIAL;
    letter['#'].special = 91;
    letter['#'].random = RANDOM_EGO;
    letter['#'].cave_info = CAVE_GLOW;
    const auto original = letter['#'];
    CHECK(apply_town_map_feature(floor, definition.features.front()) == PARSE_ERROR_INVALID_FLAG);
    check_same_grid(letter['#'], original);
}
