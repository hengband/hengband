#include "artifact/fixed-art-types.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/jsonc-document-loader.h"
#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "info-reader/town-map-reader.h"
#include "io/condition-expression.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
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
#include "test/info-reader/legend-ego-fixture.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/terrain-list-test-access.h"
#include "util/finalizer.h"
#include "view/display-messages.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <doctest/doctest.h>
#include <filesystem>
#include <fmt/format.h>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
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
        , terrain_state(TerrainList::get_instance())
        , none_tag(TerrainTag::NONE, 0)
    {
        std::copy(std::begin(letter), std::end(letter), saved_letters.begin());
        init_flags = static_cast<init_flags_type>(0);
        auto &terrains = TerrainList::get_instance();
        terrains.resize(3);
        terrains.get_terrain(0).tag = "NONE";
        terrains.get_terrain(1).tag = "TOWN_FEATURE_TEST_FLOOR";
        terrains.get_terrain(2).tag = "TOWN_FEATURE_TEST_TRAP";
    }

    ~TownFeatureStateGuard()
    {
        std::copy(saved_letters.begin(), saved_letters.end(), std::begin(letter));
        init_flags = saved_init_flags;
    }

    TownFeatureStateGuard(const TownFeatureStateGuard &) = delete;
    TownFeatureStateGuard &operator=(const TownFeatureStateGuard &) = delete;

    short floor_id() const
    {
        return 1;
    }
    short trap_id() const
    {
        return 2;
    }

private:
    init_flags_type saved_init_flags;
    std::array<dungeon_grid, 255> saved_letters;
    test::ScopedVectorWrapper<TerrainList> terrain_state;
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
        { "version", 3 },
        { "featureRules", { { { "symbol", "#" }, { "definition", { { "terrain", "WALL" }, { "caveInfo", nlohmann::json::array() }, { "trap", "NONE" }, { "special", 0 } } } } } },
        { "buildingRules", { { { "index", 1 }, { "locale", "en" }, { "command", "N" }, { "fields", { "Building", "Owner", "Race" } } } } },
        { "mapVariants", { { { "rows", { "###", "#.#", "###" } } } } },
        { "startingPositions", { { { "y", 1 }, { "x", 1 } } } },
    };
}

TownMapDefinition read_reward_town_map(FloorType &floor, ArtifactDefinition artifact, bool artifact_only_with_trap = false)
{
    ArtifactList::get_instance().emplace(FixedArtifactId::GALADRIEL_PHIAL, std::move(artifact));
    floor.quest_number = QuestId::THIEF;
    auto data = make_town_map();
    auto &fields = data["featureRules"][0]["definition"];
    fields["terrain"] = "TOWN_FEATURE_TEST_FLOOR";
    fields["trap"] = artifact_only_with_trap ? "TOWN_FEATURE_TEST_TRAP" : "NONE";
    if (!artifact_only_with_trap) {
        fields["object"] = { { "questReward", true } };
    }
    fields["artifact"] = { { "questReward", true } };
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    return definition;
}

tl::expected<void, ParseErrorType> read_town(const nlohmann::json &data, bool only_buildings = false)
{
    TownMapDefinition definition;
    return TownMapReader(data).read(definition, 3, 3, only_buildings);
}
}

TEST_CASE("TownMapReader shares legend cave flag names")
{
    test::ScopedReaderState reader_state;
    for (const auto *name : { "MARK", "GLOW", "NO_TELEPORT_DEST", "ROOM", "LITE" }) {
        CAPTURE(name);
        REQUIRE(is_legend_cave_flag(name));
        auto data = make_town_map();
        auto &fields = data["featureRules"][0]["definition"];
        fields["caveInfo"] = nlohmann::json::array();
        fields["caveInfo"].push_back(name);
        DeferredQuestLegendCell parsed;
        REQUIRE(parse_quest_legend_cell_deferred(fields, parsed).has_value());
        TownMapDefinition town;
        REQUIRE(TownMapReader(data).read(town, 3, 3).has_value());
        REQUIRE(town.features.size() == 1);
        CHECK(town.features.front().cell.grid.cave_info == parsed.cell.grid.cave_info);
        CHECK(parsed.cell.grid.cave_info != 0);
    }
    auto data = make_town_map();
    data["featureRules"][0]["definition"]["caveInfo"] = { "UNKNOWN" };
    CHECK_FALSE(is_legend_cave_flag("UNKNOWN"));
    const auto result = read_town(data);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == ParseErrorType::INVALID_FLAG);
}

TEST_CASE("TownMapReader preserves scalar legend boundaries")
{
    test::ScopedReaderState reader_state;
    test::LegendEgoFixture fixture;
    // 境界のエゴIDも有効な剣用定義として用意し、数値検証後の存在・相性検証を通す。
    for (const auto id : { 1, 255 }) {
        const auto ego_id = static_cast<EgoType>(id);
        auto &ego = egos_info[ego_id];
        ego.idx = ego_id;
        ego.slot = INVEN_MAIN_HAND;
        ego.rating = 10;
    }
    struct Boundary {
        const char *field;
        int minimum;
        int maximum;
    };
    for (const auto boundary : { Boundary{ "special", -32768, 32767 }, Boundary{ "monster", 1, 32767 },
             Boundary{ "object", 1, 32767 }, Boundary{ "artifact", 1, 32767 }, Boundary{ "ego", 1, 255 } }) {
        CAPTURE(boundary.field);
        for (const int value : { boundary.minimum - 1, boundary.minimum, boundary.maximum, boundary.maximum + 1 }) {
            CAPTURE(value);
            auto data = make_town_map();
            auto &fields = data["featureRules"][0]["definition"];
            if (std::string_view(boundary.field) == "ego") {
                fields["object"] = 42;
            }
            fields[boundary.field] = value;
            const auto result = read_town(data);
            if (value >= boundary.minimum && value <= boundary.maximum) {
                CHECK(test::to_error_code(result) == ParseErrorType::NONE);
            } else {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == ParseErrorType::INVALID_VALUE);
            }
        }
    }
}

TEST_CASE("TownMapReader preserves structured legend boundaries")
{
    test::ScopedReaderState reader_state;
    struct Boundary {
        const char *field;
        const char *key;
        int minimum;
    };
    for (const auto boundary : { Boundary{ "monster", "cloneOf", 1 }, Boundary{ "monster", "oodLevel", 0 },
             Boundary{ "object", "oodLevel", 0 }, Boundary{ "artifact", "id", 1 } }) {
        CAPTURE(boundary.field);
        CAPTURE(boundary.key);
        for (const int value : { boundary.minimum - 1, boundary.minimum, 32767, 32768 }) {
            CAPTURE(value);
            auto data = make_town_map();
            auto &spec = data["featureRules"][0]["definition"][boundary.field];
            spec = nlohmann::json::object();
            if (std::string_view(boundary.key) != "cloneOf") {
                spec["random"] = true;
            }
            spec[boundary.key] = value;
            const auto result = read_town(data);
            if (value >= boundary.minimum && value <= 32767) {
                CHECK(test::to_error_code(result) == ParseErrorType::NONE);
            } else {
                REQUIRE_FALSE(result.has_value());
                CHECK(result.error() == ParseErrorType::INVALID_VALUE);
            }
        }
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
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
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
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
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
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    data["featureRules"][0]["symbol"] = ".";
    data["startingPositions"][0]["x"] = 3;
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_VALUE);
    CHECK(definition.features[0].symbol == '#');
    CHECK(definition.starts[0].x == 1);
    data["startingPositions"][0]["x"] = 2;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.features.size() == 1);
    CHECK(definition.features[0].symbol == '.');
    CHECK(definition.starts[0].x == 2);
}

TEST_CASE("TownMapReader validates root and conditions on every rule")
{
    for (const auto *field : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        auto data = make_town_map();
        data.erase(field);
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        data = make_town_map();
        data[field] = nullptr;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        for (const auto &condition : { nlohmann::json(nullptr), nlohmann::json(false), nlohmann::json("") }) {
            data = make_town_map();
            data[field][0]["when"] = condition;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        }
    }
    for (const auto &version : { nlohmann::json(1), nlohmann::json(2.0), nlohmann::json("2") }) {
        auto data = make_town_map();
        data["version"] = version;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
    }
    for (const auto *field : { "mapVariants", "startingPositions" }) {
        auto data = make_town_map();
        data[field] = nlohmann::json::array();
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("TownMapReader validates v3 modes types and narrowing bounds")
{
    const test::ScopedReaderState reader_state;
    for (const auto *field : { "monster", "object", "artifact", "ego" }) {
        for (const auto &value : std::array<nlohmann::json, 4>{ { 1, 255, { { "random", true } }, nullptr } }) {
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = value;
            // エゴ単独では配置対象がなく、固定ID 1/255 も未定義なので拒否する。
            const auto expected = value.is_null()                    ? ParseErrorType::INVALID_TYPE
                                  : std::string_view(field) == "ego" ? ParseErrorType::INVALID_VALUE
                                                                     : ParseErrorType::NONE;
            CHECK(test::to_error_code(read_town(data)) == expected);
        }
        for (const auto &value : std::array<nlohmann::json, 6>{ { 0, -1, 32768, { { "random", false } }, { { "random", true }, { "questReward", true } }, nlohmann::json::object() } }) {
            auto data = make_town_map();
            data["featureRules"][0]["when"] = "0";
            data["featureRules"][0]["definition"][field] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        }
        for (const auto &value : std::array<nlohmann::json, 5>{ { "1", 1.0, true, nullptr, nlohmann::json::array() } }) {
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        }
    }
}

TEST_CASE("TownMapReader ego is optional and validates placed equipment")
{
    const test::LegendEgoFixture ego_state;
    auto data = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.features.front().cell.grid.ego == EgoType::NONE);
    data["featureRules"][0]["definition"]["object"] = 42;
    for (const int id : { static_cast<int>(EgoType::DF), static_cast<int>(EgoType::BRAND_FIRE) }) {
        data["featureRules"][0]["definition"]["ego"] = id;
        REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
        CHECK(enum2i(definition.features.front().cell.grid.ego) == id);
    }
    data["featureRules"][0]["definition"]["ego"] = { { "random", true } };
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.features.front().cell.grid.random == RANDOM_EGO);
    for (const auto &value : std::array<nlohmann::json, 5>{ { 0, -1, 256, { { "random", true }, { "id", 1 } }, { { "random", false } } } }) {
        auto invalid = data;
        invalid["featureRules"][0]["definition"]["ego"] = value;
        for (const bool only_buildings : { false, true }) {
            CHECK(test::to_error_code(TownMapReader(invalid).read(definition, 3, 3, only_buildings)) == ParseErrorType::INVALID_VALUE);
            CHECK(definition.features.front().cell.grid.random == RANDOM_EGO);
        }
    }
}

TEST_CASE("TownMapReader rejects integer caveInfo and out of range special and coordinates")
{
    for (const auto &value : { nlohmann::json(0.0), nlohmann::json(std::numeric_limits<uint64_t>::max()), nlohmann::json("0") }) {
        auto data = make_town_map();
        data["featureRules"][0]["definition"]["caveInfo"] = value;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
    }
    auto data = make_town_map();
    data["featureRules"][0]["definition"]["caveInfo"] = { "MARK", "GLOW", "NO_TELEPORT_DEST", "ROOM", "LITE" };
    for (const int value : { -32768, 32767 }) {
        data["featureRules"][0]["definition"]["special"] = value;
        CHECK(read_town(data).has_value());
    }
    for (const int value : { -32769, 32768 }) {
        CAPTURE(value);
        data["featureRules"][0]["definition"]["special"] = value;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    // v3のcaveInfoは名前の配列なので、整数は値によらず型エラーになる。
    for (const int value : { -1, std::numeric_limits<uint16_t>::max() + 1 }) {
        CAPTURE(value);
        data = make_town_map();
        data["featureRules"][0]["definition"]["caveInfo"] = value;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
    }
    for (const auto *coordinate : { "x", "y" }) {
        for (const auto &value : { nlohmann::json(-1), nlohmann::json(3), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
            data = make_town_map();
            data["startingPositions"][0][coordinate] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        }
    }
}

TEST_CASE("TownMapReader validates building commands and numeric fields")
{
    auto data = make_town_map();
    auto &rule = data["buildingRules"][0];
    for (const auto &index : { nlohmann::json(-1), nlohmann::json(MAX_BUILDINGS), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        rule["index"] = index;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    rule["index"] = MAX_BUILDINGS - 1;
    rule["command"] = "A";
    rule["fields"] = { "7", "Action", "0", "-1", "a", "0", "0" };
    CHECK(read_town(data).has_value());
    for (const auto *index : { "-1", "8", "2147483648" }) {
        rule["fields"][0] = index;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    rule["fields"][0] = "0";
    for (const int index : { 2, 3, 5, 6 }) {
        rule["fields"][index] = "2147483648";
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        rule["fields"][index] = "0";
    }
    for (const auto &[command, count] : { std::pair{ "C", static_cast<int>(PLAYER_CLASS_TYPE_MAX) }, std::pair{ "M", static_cast<int>(MAX_MAGIC) }, std::pair{ "R", MAX_RACES } }) {
        rule["command"] = command;
        rule["fields"] = std::vector<std::string>(count, "0");
        CHECK(read_town(data).has_value());
        rule["fields"].push_back("0");
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        rule["fields"] = nlohmann::json::array();
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
}

TEST_CASE("TownMapReader preserves JSON integer error categories and output on failure")
{
    struct Field {
        const char *section;
        const char *name;
        ParseErrorType range_error;
    };
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3).has_value());
    for (const auto &field : {
             Field{ "buildingRules", "index", ParseErrorType::INVALID_VALUE },
             Field{ "startingPositions", "x", ParseErrorType::INVALID_VALUE },
             Field{ "startingPositions", "y", ParseErrorType::INVALID_VALUE },
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
            const auto expected = value.is_number_integer() ? field.range_error : ParseErrorType::INVALID_TYPE;
            for (const bool missing : { false, true }) {
                CAPTURE(missing);
                if (missing) {
                    fields.erase(field.name);
                }
                CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == (missing ? ParseErrorType::INVALID_TYPE : expected));
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
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    auto data = make_town_map();
    data["buildingRules"][0]["fields"] = { "Building", "Owner" };
    CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    data = make_town_map();
    data["buildingRules"][0]["locale"] = "other";
    CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    data["buildingRules"][0]["locale"] = "en";
    data["buildingRules"][0]["command"] = "Unknown";
    CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
}

TEST_CASE("TownMapReader validates rectangular printable maps and all start bounds")
{
    for (const auto &rows : { nlohmann::json{ "###", "##" }, nlohmann::json{ "" }, nlohmann::json{ "####" }, nlohmann::json{ "###", "###", "###", "###" }, nlohmann::json{ "##\t" }, nlohmann::json{ "\xE7\x94\xBA" }, nlohmann::json{ 3 } }) {
        CAPTURE(rows.dump(-1, ' ', true));
        auto data = make_town_map();
        data["mapVariants"][0]["when"] = "0";
        data["mapVariants"][0]["rows"] = rows;
        const auto oversize = rows.size() > 3 || (rows.front().is_string() && rows.front().get_ref<const std::string &>().size() > 3);
        CHECK(test::to_error_code(read_town(data)) == (oversize ? ParseErrorType::OUT_OF_BOUNDS : ParseErrorType::INVALID_VALUE));
    }
    auto data = make_town_map();
    data["mapVariants"].push_back({ { "when", "0" }, { "rows", { "#" } } });
    CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    data["startingPositions"][0]["x"] = 0;
    data["startingPositions"][0]["y"] = 0;
    CHECK(read_town(data).has_value());
    TownMapDefinition definition;
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 0, 3)) == ParseErrorType::OUT_OF_BOUNDS);
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 0)) == ParseErrorType::OUT_OF_BOUNDS);
}

TEST_CASE("TownMapReader building-only mode skips map and start contents, not root requirements")
{
    auto data = make_town_map();
    data["mapVariants"] = { nullptr };
    data["startingPositions"] = { false };
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true).has_value());
    CHECK(definition.features.size() == 1);
    CHECK(definition.buildings.size() == 1);
    CHECK(definition.maps.empty());
    CHECK(definition.starts.empty());
    data["featureRules"][0]["symbol"] = ":";
    CHECK(test::to_error_code(read_town(data, true)) == ParseErrorType::INVALID_VALUE);
    data = make_town_map();
    data["buildingRules"][0]["command"] = "?";
    CHECK(test::to_error_code(read_town(data, true)) == ParseErrorType::INVALID_VALUE);
    data = make_town_map();
    data["mapVariants"] = nlohmann::json::array();
    CHECK(test::to_error_code(read_town(data, true)) == ParseErrorType::INVALID_VALUE);
    data = make_town_map();
    data.erase("startingPositions");
    CHECK(test::to_error_code(read_town(data, true)) == ParseErrorType::INVALID_TYPE);
}

TEST_CASE("TownMapReader accepts unsigned integers produced by JSON parsing")
{
    auto input = make_town_map();
    input["featureRules"][0]["definition"]["caveInfo"] = { "MARK", "GLOW", "NO_TELEPORT_DEST", "ROOM", "LITE" };
    input["featureRules"][0]["definition"]["special"] = 32767;
    input["buildingRules"][0]["index"] = MAX_BUILDINGS - 1;
    input["startingPositions"][0]["x"] = 2;
    input["startingPositions"][0]["y"] = 2;
    auto data = nlohmann::json::parse(input.dump());
    REQUIRE(data["version"].is_number_unsigned());
    REQUIRE(data["featureRules"][0]["definition"]["caveInfo"].is_array());
    REQUIRE(data["featureRules"][0]["definition"]["special"].is_number_unsigned());
    REQUIRE(data["buildingRules"][0]["index"].is_number_unsigned());
    REQUIRE(data["startingPositions"][0]["x"].is_number_unsigned());
    REQUIRE(data["startingPositions"][0]["y"].is_number_unsigned());
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.features[0].cell.grid.cave_info == 31);
    CHECK(definition.features[0].cell.grid.special == 32767);
    CHECK(definition.buildings[0].index == MAX_BUILDINGS - 1);
    CHECK(definition.starts[0].x == 2);
    CHECK(definition.starts[0].y == 2);

    input["featureRules"][0]["definition"]["caveInfo"] = nlohmann::json::array();
    input["featureRules"][0]["definition"]["special"] = 0;
    input["buildingRules"][0]["index"] = 0;
    input["startingPositions"][0]["x"] = 0;
    input["startingPositions"][0]["y"] = 0;
    data = nlohmann::json::parse(input.dump());
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
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
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        rule["fields"][0] = "0";
        for (const int index : { 1, 2, 3, 4, 5, 6 }) {
            CAPTURE(index);
            rule["fields"][index] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
            rule["fields"][index] = "0";
        }
    }
}

TEST_CASE("TownMapReader leaves all output intact on failure at every validation stage")
{
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3).has_value());
    for (const auto *stage : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
        CAPTURE(std::string(stage));
        auto data = original;
        data["featureRules"][0]["symbol"] = ".";
        data["buildingRules"][0]["fields"][0] = "Changed";
        data["mapVariants"][0]["rows"][0] = "...";
        data["startingPositions"][0]["x"] = 2;
        data[stage].push_back(nullptr);
        CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_TYPE);
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
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    REQUIRE_FALSE(definition.maps.empty());
    REQUIRE_FALSE(definition.starts.empty());
    data["featureRules"][0]["symbol"] = ".";
    data["buildingRules"][0]["fields"][0] = "Changed";
    for (const auto &invalid : { nlohmann::json(nullptr), nlohmann::json{ { "index", -1 }, { "locale", "en" }, { "command", "Z" }, { "fields", nlohmann::json::array() } } }) {
        const auto expected = invalid.is_null() ? ParseErrorType::INVALID_TYPE : ParseErrorType::INVALID_VALUE;
        CAPTURE(invalid);
        auto bad = data;
        bad["buildingRules"].push_back(invalid);
        CHECK(test::to_error_code(TownMapReader(bad).read(definition, 0, 0, true)) == expected);
        REQUIRE(definition.features.size() == 1);
        REQUIRE(definition.buildings.size() == 1);
        REQUIRE(definition.maps.size() == 1);
        REQUIRE(definition.starts.size() == 1);
        CHECK(definition.features[0].symbol == '#');
        CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
        CHECK(definition.maps[0].rows[0] == "###");
        CHECK(definition.starts[0].x == 1);
    }
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true).has_value());
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
            CHECK(test::to_error_code(TownMapReader(data).read(definition, 10, 10)) == ParseErrorType::INVALID_VALUE);
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
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    for (const auto *symbol : { " ", "~" }) {
        CAPTURE(hex_bytes(symbol));
        auto data = make_town_map();
        data["featureRules"][0]["symbol"] = symbol;
        CHECK(read_town(data).has_value());
    }
    for (const auto *field : { "terrain", "monster", "object", "ego", "artifact", "trap" }) {
        CAPTURE(std::string(field));
        auto data = make_town_map();
        data["featureRules"][0]["definition"][field] = nullptr;
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        data["featureRules"][0]["definition"][field] = "bad/field";
        const bool tag = std::string_view(field) == "terrain" || std::string_view(field) == "trap";
        CHECK(test::to_error_code(read_town(data)) == (tag ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE));
        data["featureRules"][0]["definition"][field] = 0;
        CHECK(test::to_error_code(read_town(data)) == (tag ? ParseErrorType::INVALID_TYPE : ParseErrorType::INVALID_VALUE));
        data["featureRules"][0]["definition"].erase(field);
        CHECK(read_town(data).has_value());
    }
}

TEST_CASE("TownMapReader rejects parsed unsigned integers immediately above each upper bound")
{
    struct Boundary {
        const char *section;
        const char *field;
        uint64_t value;
        ParseErrorType error;
    };
    for (const auto &boundary : {
             Boundary{ "featureRules", "special", 32768, ParseErrorType::INVALID_VALUE },
             Boundary{ "featureRules", "caveInfo", static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()) + 1, ParseErrorType::INVALID_TYPE },
             Boundary{ "buildingRules", "index", static_cast<uint64_t>(MAX_BUILDINGS), ParseErrorType::INVALID_VALUE },
             Boundary{ "startingPositions", "x", 3, ParseErrorType::INVALID_VALUE },
             Boundary{ "startingPositions", "y", 3, ParseErrorType::INVALID_VALUE },
         }) {
        CAPTURE(std::string(boundary.section));
        CAPTURE(std::string(boundary.field));
        CAPTURE(boundary.value);
        auto data = make_town_map();
        auto &record = data[boundary.section][0];
        auto &value = std::string_view(boundary.section) == "featureRules" ? record["definition"][boundary.field] : record[boundary.field];
        value = nlohmann::json::parse(std::to_string(boundary.value));
        REQUIRE(value.is_number_unsigned());
        CHECK(test::to_error_code(read_town(data)) == boundary.error);
    }
}

TEST_CASE("TownMapReader rejects wrong action field counts and malformed numeric fields")
{
    for (const size_t count : { 0U, 6U, 8U }) {
        CAPTURE(count);
        auto data = make_town_map();
        data["buildingRules"][0]["command"] = "A";
        data["buildingRules"][0]["fields"] = std::vector<std::string>(count, "0");
        CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
    }
    for (const int index : { 0, 2, 3, 5, 6 }) {
        CAPTURE(index);
        for (const auto *value : { "", "1x", " 1" }) {
            CAPTURE(hex_bytes(value));
            auto data = make_town_map();
            data["buildingRules"][0]["command"] = "A";
            data["buildingRules"][0]["fields"] = { "0", "Action", "0", "0", "a", "0", "0" };
            data["buildingRules"][0]["fields"][index] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
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
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_TYPE);
        }
    }
}

TEST_CASE("TownMapReader preserves output on root errors and value errors at every stage")
{
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3).has_value());
    for (const auto *failure : { "version", "emptyMaps", "missingFeatures", "symbol", "locale", "row", "coordinate" }) {
        CAPTURE(std::string(failure));
        auto data = original;
        data["featureRules"][0]["symbol"] = ".";
        data["buildingRules"][0]["fields"][0] = "Changed";
        data["mapVariants"][0]["rows"][0] = "...";
        data["startingPositions"][0]["x"] = 2;
        ParseErrorType expected = ParseErrorType::INVALID_VALUE;
        const std::string_view kind = failure;
        if (kind == "version") {
            data["version"] = 1;
            expected = ParseErrorType::INVALID_TYPE;
        } else if (kind == "emptyMaps") {
            data["mapVariants"] = nlohmann::json::array();
        } else if (kind == "missingFeatures") {
            data.erase("featureRules");
            expected = ParseErrorType::INVALID_TYPE;
        } else if (kind == "symbol") {
            data["featureRules"][0]["symbol"] = ":";
        } else if (kind == "locale") {
            data["buildingRules"][0]["locale"] = "other";
        } else if (kind == "row") {
            data["mapVariants"][0]["rows"][0] = "##";
        } else {
            data["startingPositions"][0]["x"] = 3;
        }
        CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == expected);
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

/*!
 * @brief 複数の段階に誤りがあるとき、凡例・建物・マップ・開始位置の順で最初の段階のエラーを返す
 * @details 隣り合う段階で異なるエラーコードになる誤りを選び、順序の入れ替わりを検出する。
 */
TEST_CASE("TownMapReader reports the first failing stage in declaration order")
{
    auto data = make_town_map();
    data["featureRules"][0]["when"] = "";
    data["buildingRules"][0]["locale"] = "other";
    data["mapVariants"][0]["when"] = "";
    data["startingPositions"][0]["x"] = 3;
    TownMapDefinition definition;
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_TYPE);
    data["featureRules"][0].erase("when");
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_VALUE);
    data["buildingRules"][0]["locale"] = "en";
    CHECK(TownMapReader(data).read(definition, 3, 3, true).has_value());
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_TYPE);
    data["mapVariants"][0].erase("when");
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_VALUE);
    data["startingPositions"][0]["x"] = 1;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.starts.size() == 1);
}

TEST_CASE("TownMapReader rejects nonprintable terrain and trap tokens")
{
    for (const auto *field : { "terrain", "trap" }) {
        CAPTURE(std::string(field));
        for (const auto &value : { std::string("WALL\t"), std::string("\x1f"), std::string("\x7f"), std::string("\xE7\x94\xBA"), std::string("WALL\0", 5) }) {
            CAPTURE(hex_bytes(value));
            auto data = make_town_map();
            data["featureRules"][0]["definition"][field] = value;
            CHECK(test::to_error_code(read_town(data)) == ParseErrorType::INVALID_VALUE);
        }
    }
}

TEST_CASE("TownMapReader typed building application preserves N A C R M Z contracts in both locales")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    const auto names = [&initial](std::string_view name, std::string_view owner, std::string_view race) {
        auto expected = initial;
        REQUIRE(name.size() < sizeof(expected.name));
        REQUIRE(owner.size() < sizeof(expected.owner_name));
        REQUIRE(race.size() < sizeof(expected.owner_race));
        std::copy(name.begin(), name.end(), expected.name);
        std::copy(owner.begin(), owner.end(), expected.owner_name);
        std::copy(race.begin(), race.end(), expected.owner_race);
        return expected;
    };
    const auto town = std::string("\xE7\x94\xBA");
    const auto short_name = utf8_to_sys("\xE7\x94\xBA\xE3\x81\xAE\xE5\xBB\xBA\xE7\x89\xA9");
    REQUIRE(short_name.has_value());
    std::string repeated_name;
    for (int i = 0; i < 10; ++i) {
        repeated_name += town;
    }
    // 日本語版は2バイト文字を分断せず9文字で止め、英語版はUTF-8の19バイトを保持する。
#ifdef JP
    const auto truncated_name = utf8_to_sys(repeated_name.substr(0, 27));
#else
    const auto truncated_name = utf8_to_sys(repeated_name.substr(0, 19));
#endif
    REQUIRE(truncated_name.has_value());
    auto action = initial;
    const std::string action_name = "Action name that is longer th";
    std::copy(action_name.begin(), action_name.end(), action.act_names[7]);
    action.member_costs[7] = std::numeric_limits<int>::max();
    action.other_costs[7] = std::numeric_limits<int>::min();
    action.letters[7] = 'a';
    action.actions[7] = -1;
    action.action_restr[7] = 32767;
    auto classes = initial;
    std::fill(classes.member_class.begin(), classes.member_class.end(), short{ 1 });
    classes.member_class[0] = 2;
    classes.member_class[1] = 0;
    auto races = initial;
    std::fill(races.member_race.begin(), races.member_race.end(), short{ 1 });
    races.member_race[0] = 0;
    races.member_race[1] = -1;
    auto realms = initial;
    std::fill(realms.member_realm.begin() + 1, realms.member_realm.end(), short{ 1 });
    realms.member_realm[1] = 3;
    realms.member_realm[2] = -1;
    struct BuildingCase {
        const char *command;
        std::vector<std::string> fields;
        building_type expected;
    };
    const std::vector<BuildingCase> cases{
        { "N", { "Building name that is longer than nineteen bytes", "Owner", "Race" }, names("Building name that ", "Owner", "Race") },
        { "N", { "", "", "" }, initial },
        { "N", { "\xE7\x94\xBA\xE3\x81\xAE\xE5\xBB\xBA\xE7\x89\xA9", "Owner", "Race" }, names(*short_name, "Owner", "Race") },
        { "N", { repeated_name, "Owner", "Race" }, names(*truncated_name, "Owner", "Race") },
        { "A", { "7", "Action name that is longer than twenty nine bytes", "2147483647", "-2147483648", "abc", "-1", "32767" }, action },
        { "A", { "0", "", "0", "0", "", "0", "0" }, initial },
        { "C", { "2", "0" }, classes },
        { "R", { "0", "-1" }, races },
        { "M", { "3", "-1" }, realms },
        { "Z", { "Ignored", "\xE7\x94\xBA" }, initial },
        { "Z", {}, initial },
    };
    for (const auto *locale : { "en", "ja" }) {
        CAPTURE(std::string(locale));
        for (const auto &test_case : cases) {
            CAPTURE(std::string(test_case.command));
            auto data = make_town_map();
            data["buildingRules"][0]["locale"] = locale;
            data["buildingRules"][0]["command"] = test_case.command;
            data["buildingRules"][0]["fields"] = test_case.fields;
            TownMapDefinition definition;
            REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
            REQUIRE(definition.buildings.size() == 1);
            buildings[1] = initial;
            REQUIRE(apply_town_building_rule(definition.buildings[0]).has_value());
            check_building(buildings[1], std::string_view(locale) == _("ja", "en") ? test_case.expected : initial);
            check_building(buildings[2], initial);
        }
    }
}

TEST_CASE("TownMapReader rejects building fields outside their storage range before publishing")
{
    const auto original = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(original).read(definition, 3, 3).has_value());
    for (const auto *locale : { "en", "ja" }) {
        for (const auto number : { -32769, 32768, 65535, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
            for (const auto *command : { "A", "C", "R", "M" }) {
                const auto indices = std::string_view(command) == "A" ? std::vector<size_t>{ 5, 6 } : std::vector<size_t>{ 1 };
                for (const auto field_index : indices) {
                    CAPTURE(std::string(locale));
                    CAPTURE(number);
                    CAPTURE(std::string(command));
                    CAPTURE(field_index);
                    auto data = original;
                    auto &rule = data["buildingRules"][0];
                    rule["locale"] = locale;
                    rule["when"] = "0";
                    rule["command"] = command;
                    if (std::string_view(command) == "A") {
                        rule["fields"] = { "0", "Changed", "1", "2", "a", "3", "4" };
                        rule["fields"][field_index] = std::to_string(number);
                    } else {
                        rule["fields"] = { "2", std::to_string(number) };
                    }
                    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_VALUE);
                    REQUIRE(definition.buildings.size() == 1);
                    REQUIRE(std::holds_alternative<TownMapBuildingNames>(definition.buildings[0].directive));
                    CHECK(std::get<TownMapBuildingNames>(definition.buildings[0].directive).name == "Building");
                    REQUIRE(definition.features.size() == 1);
                    CHECK(definition.features[0].symbol == original["featureRules"][0]["symbol"].get<std::string>().front());
                    REQUIRE(definition.maps.size() == 1);
                    CHECK(definition.maps[0].rows == original["mapVariants"][0]["rows"].get<std::vector<std::string>>());
                    REQUIRE(definition.starts.size() == 1);
                    CHECK(definition.starts[0].x == original["startingPositions"][0]["x"].get<int>());
                    CHECK(definition.starts[0].y == original["startingPositions"][0]["y"].get<int>());
                }
            }
        }
    }
}

TEST_CASE("TownMapReader preserves building signed sixteen bit boundaries and int costs")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    for (const auto number : { -32768, 0, 32767 }) {
        for (const auto *command : { "A", "C", "R", "M" }) {
            CAPTURE(number);
            CAPTURE(std::string(command));
            auto data = make_town_map();
            auto &rule = data["buildingRules"][0];
            rule["locale"] = _("ja", "en");
            rule["command"] = command;
            if (std::string_view(command) == "A") {
                rule["fields"] = { "0", "Action", "2147483647", "-2147483648", "a", std::to_string(number), std::to_string(number) };
            } else {
                rule["fields"] = { std::to_string(number) };
            }
            TownMapDefinition definition;
            REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
            buildings[1] = initial;
            REQUIRE(apply_town_building_rule(definition.buildings[0]).has_value());
            const auto &building = buildings[1];
            if (std::string_view(command) == "A") {
                CHECK(building.actions[0] == number);
                CHECK(building.action_restr[0] == number);
                CHECK(building.member_costs[0] == std::numeric_limits<int>::max());
                CHECK(building.other_costs[0] == std::numeric_limits<int>::min());
            } else {
                const auto &members = std::string_view(command) == "C" ? building.member_class : (std::string_view(command) == "R" ? building.member_race : building.member_realm);
                const auto offset = std::string_view(command) == "M" ? 1U : 0U;
                CHECK(members[offset] == number);
                if (offset != 0) {
                    CHECK(members[0] == initial.member_realm[0]);
                }
            }
        }
    }
}

TEST_CASE("TownMapReader typed building application rejects narrowing before any writes")
{
    const ScopedTownBuildings state;
    const auto initial = buildings[1];
    auto data = make_town_map();
    data["buildingRules"][0]["locale"] = _("ja", "en");
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    auto rule = definition.buildings[0];
    for (const auto number : { -32769, 32768, 65535, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
        for (const auto restriction : { false, true }) {
            CAPTURE(number);
            CAPTURE(restriction);
            buildings[1] = initial;
            rule.directive = TownMapBuildingAction{ 0, "Changed", 1, 2, "a", restriction ? 3 : number, restriction ? number : 4 };
            CHECK(test::to_error_code(apply_town_building_rule(rule)) == ParseErrorType::INVALID_VALUE);
            check_building(buildings[1], initial);
            check_building(buildings[2], initial);
        }
        for (const auto kind : { TownMapBuildingMembershipKind::CLASS, TownMapBuildingMembershipKind::RACE, TownMapBuildingMembershipKind::REALM }) {
            buildings[1] = initial;
            rule.directive = TownMapBuildingMembership{ kind, { 2, number } };
            CHECK(test::to_error_code(apply_town_building_rule(rule)) == ParseErrorType::INVALID_VALUE);
            check_building(buildings[1], initial);
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
        REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
        REQUIRE(apply_town_building_rule(definition.buildings[0]).has_value());
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
            REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
            const auto expected = utf8_to_sys(invalid_utf8) ? ParseErrorType::NONE : ParseErrorType::INVALID_VALUE;
            CHECK(test::to_error_code(apply_town_building_rule(definition.buildings[0])) == expected);
            // 英語版は文字コード変換が恒等変換のため、対象言語のN/Aは適用される。
            if (expected != ParseErrorType::NONE || std::string_view(locale) != _("ja", "en") || std::string_view(command) == "Z") {
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
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    auto rule = definition.buildings[0];
    rule.index = MAX_BUILDINGS;
    CHECK(test::to_error_code(apply_town_building_rule(rule)) == ParseErrorType::INVALID_VALUE);
    rule.index = 1;
    rule.directive = TownMapBuildingAction{ 8, "Name", 1, 2, "a", 3, 4 };
    CHECK(test::to_error_code(apply_town_building_rule(rule)) == ParseErrorType::INVALID_VALUE);
    rule.directive = TownMapBuildingMembership{ TownMapBuildingMembershipKind::CLASS, {} };
    CHECK(test::to_error_code(apply_town_building_rule(rule)) == ParseErrorType::INVALID_VALUE);
    check_building(buildings[1], initial);
    data["buildingRules"].push_back(data["buildingRules"][0]);
    data["buildingRules"][1]["command"] = "A";
    data["buildingRules"][1]["fields"] = { "0", "Action", "0", "0", "a", "overflow", "0" };
    CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3)) == ParseErrorType::INVALID_VALUE);
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
            REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
            CHECK(test::to_error_code(apply_town_building_rule(definition.buildings[0])) == ParseErrorType::INVALID_VALUE);
            check_building(buildings[1], initial);
        }
    }
}

TEST_CASE("TownMapReader applies typed feature values and random flags")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    for (const bool random : { false, true }) {
        auto data = make_town_map();
        auto &fields = data["featureRules"][0]["definition"];
        fields["terrain"] = "TOWN_FEATURE_TEST_FLOOR";
        fields["trap"] = "NONE";
        fields["caveInfo"] = { "MARK", "GLOW", "NO_TELEPORT_DEST", "ROOM", "LITE" };
        fields["special"] = -32768;
        for (const auto *field : { "monster", "object", "artifact" }) {
            fields[field] = random ? nlohmann::json{ { "random", true } } : nlohmann::json(32767);
        }
        TownMapDefinition definition;
        REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
        const short value = random ? short{ 0 } : short{ 32767 };
        const dungeon_grid expected{ .feature = state.floor_id(), .monster = value, .object = value, .ego = EgoType::NONE, .artifact = i2enum<FixedArtifactId>(value), .trap = 0, .cave_info = 31, .special = -32768, .random = random ? RANDOM_MONSTER | RANDOM_OBJECT | RANDOM_ARTIFACT : 0 };
        letter['#'] = {};
        REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
        check_same_grid(letter['#'], expected);
    }
}

TEST_CASE("TownMapReader typed features reject unsafe bytes before touching letters")
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
        CHECK(test::to_error_code(apply_town_map_feature(floor, feature)) == ParseErrorType::INVALID_VALUE);
        for (size_t i = 0; i < original.size(); ++i) {
            check_same_grid(letter[i], original[i]);
        }
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(apply_town_map_feature(floor, feature).has_value());
        for (size_t i = 0; i < original.size(); ++i) {
            check_same_grid(letter[i], original[i]);
        }
        init_flags = static_cast<init_flags_type>(0);
    }
}

TEST_CASE("TownMapReader typed features retain printable ASCII boundaries")
{
    const TownFeatureStateGuard state;
    const FloorType floor;
    for (const auto symbol : { ' ', '~' }) {
        CAPTURE(symbol);
        TownMapFeatureRule feature;
        feature.symbol = symbol;
        feature.terrain = "TOWN_FEATURE_TEST_FLOOR";
        feature.trap = "TOWN_FEATURE_TEST_TRAP";
        REQUIRE(apply_town_map_feature(floor, feature).has_value());
        CHECK(letter[static_cast<unsigned char>(symbol)].feature == state.floor_id());
        CHECK(letter[static_cast<unsigned char>(symbol)].trap == state.trap_id());
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
    fields["trap"] = { { "random", true } };
    fields["monster"] = { { "cloneOf", 32767 } };
    fields["artifact"] = { { "questReward", true } };
    fields["special"] = 32767;
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    const dungeon_grid expected{ .feature = 0, .monster = -32767, .object = 0, .ego = EgoType::NONE, .artifact = FixedArtifactId::NONE, .trap = 0, .cave_info = 0, .special = 32767, .random = RANDOM_FEATURE | RANDOM_TRAP };
    letter['#'] = {};
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
    check_same_grid(letter['#'], expected);
    CHECK(letter['#'].monster == -32767);
    CHECK(letter['#'].random == (RANDOM_FEATURE | RANDOM_TRAP));
    CHECK_FALSE(definition.features.front().cell.object_is_quest_reward);
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
        REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
        CHECK(definition.features.front().condition == "0");
        letter['#'].feature = 123;
        letter['#'].monster = 456;
        letter['#'].random = RANDOM_EGO;
        const auto original = letter['#'];
        CHECK(test::to_error_code(apply_town_map_feature(floor, definition.features.front())) == ParseErrorType::UNDEFINED_TERRAIN_TAG);
        check_same_grid(letter['#'], original);
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(apply_town_map_feature(floor, definition.features.front()).has_value());
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
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
    CHECK(letter['#'].object == 0);
    CHECK(letter['#'].artifact == FixedArtifactId::NONE);
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);
    const dungeon_grid expected{ .feature = state.floor_id(), .monster = 0, .object = 0, .ego = EgoType::NONE, .artifact = FixedArtifactId::GALADRIEL_PHIAL, .trap = 0, .cave_info = 0, .special = 0, .random = 0 };
    letter['#'] = {};
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
    check_same_grid(letter['#'], expected);
    CHECK(letter['#'].object == 0); // Instant artifacts do not generate a base item.
    CHECK(letter['#'].artifact == FixedArtifactId::GALADRIEL_PHIAL);
    floor.quest_number = QuestId::NONE;
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
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
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
    CHECK(letter['#'].object == 7);
    CHECK(letter['#'].artifact == FixedArtifactId::GALADRIEL_PHIAL);
    auto grid = definition.features.front().cell.grid;
    resolve_legend_reward(grid, definition.features.front().cell, QuestList::get_instance().get_quest(floor.quest_number));
    CHECK(grid.object == 7);
    CHECK(grid.artifact == FixedArtifactId::GALADRIEL_PHIAL);
    CHECK(letter['#'].feature == state.floor_id());
    CHECK(letter['#'].trap == 0);
}

TEST_CASE("TownMapReader returns an error and preserves the feature when quest reward reference lookup fails")
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
    auto definition = read_reward_town_map(floor, std::move(artifact));
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);

    SUBCASE("missing reward baseitem") {}
    SUBCASE("missing reward baseitem subtype")
    {
        auto &reward = *ArtifactList::get_instance().begin();
        REQUIRE(reward.first == FixedArtifactId::GALADRIEL_PHIAL);
        reward.second.bi_key = BaseitemKey{ ItemKindType::SWORD };
        REQUIRE_THROWS_AS(QuestList::get_instance().get_quest(floor.quest_number).get_reward_bi_id(), std::logic_error);
    }
    SUBCASE("missing quest")
    {
        floor.quest_number = QuestId::SEWER;
        REQUIRE_FALSE(QuestList::get_instance().contains(floor.quest_number));
        SUBCASE("both placeholders") {}
        SUBCASE("only object placeholder")
        {
            definition.features.front().cell.artifact_is_quest_reward = false;
        }
        SUBCASE("only artifact placeholder")
        {
            definition.features.front().cell.object_is_quest_reward = false;
        }
    }
    SUBCASE("missing reward artifact")
    {
        constexpr auto undefined_reward = static_cast<FixedArtifactId>(32767);
        REQUIRE_FALSE(ArtifactList::get_instance().contains(undefined_reward));
        test::QuestFeatureTestAccess::set_resolved_reward(undefined_reward);
        SUBCASE("both placeholders") {}
        SUBCASE("only object placeholder")
        {
            definition.features.front().cell.artifact_is_quest_reward = false;
        }
        SUBCASE("only artifact placeholder")
        {
            definition.features.front().cell.object_is_quest_reward = false;
        }
    }

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
    CHECK(test::to_error_code(apply_town_map_feature(floor, definition.features.front())) == ParseErrorType::INVALID_VALUE);
    check_same_grid(letter['#'], original);
}

TEST_CASE("TownMapReader preserves an artifact-only quest reward with a fixed trap")
{
    const TownFeatureStateGuard state;
    const test::QuestFeatureTestAccess quest_state;
    const test::ArtifactListTestAccess artifact_state;
    ArtifactDefinition artifact;
    artifact.gen_flags.set(ItemGenerationTraitType::INSTA_ART);
    FloorType floor;
    const auto definition = read_reward_town_map(floor, std::move(artifact), true);
    const auto &feature = definition.features.front();
    CHECK(feature.cell.artifact_is_quest_reward);
    CHECK_FALSE(feature.cell.object_is_quest_reward);
    CHECK(feature.cell.grid.object == 0);
    CHECK(feature.trap == "TOWN_FEATURE_TEST_TRAP");
    test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);
    REQUIRE(apply_town_map_feature(floor, feature).has_value());
    CHECK(letter['#'].artifact == FixedArtifactId::GALADRIEL_PHIAL);
    CHECK(letter['#'].trap == state.trap_id());
    CHECK(letter['#'].object == 0);
    CHECK(letter['#'].feature == state.floor_id());
}

TEST_CASE("TownMapReader applies an artifact-only reward without resolving its baseitem")
{
    const TownFeatureStateGuard state;
    const test::QuestFeatureTestAccess quest_state;
    const test::ArtifactListTestAccess artifact_state;
    const test::ScopedVectorWrapper items_state(BaseitemList::get_instance());
    ArtifactDefinition artifact;
    artifact.bi_key = { ItemKindType::SWORD, 37 };
    REQUIRE_FALSE(artifact.is_instant_artifact());
    FloorType floor;
    auto definition = read_reward_town_map(floor, std::move(artifact));
    auto &cell = definition.features.front().cell;
    cell.object_is_quest_reward = false;
    cell.grid.object = 19;

    SUBCASE("unresolved reward") {}
    SUBCASE("resolved reward")
    {
        test::QuestFeatureTestAccess::set_resolved_reward(FixedArtifactId::GALADRIEL_PHIAL);
    }
    const auto reward = QuestList::get_instance().get_quest(floor.quest_number).get_reward();
    REQUIRE(apply_town_map_feature(floor, definition.features.front()).has_value());
    CHECK(letter['#'].object == 19);
    CHECK(letter['#'].artifact == reward.value_or(FixedArtifactId::NONE));
    CHECK(letter['#'].feature == state.floor_id());
    CHECK(letter['#'].trap == 0);
}

TEST_CASE("TownMapReader v3 validates precedence and preserves output and letters")
{
    TownFeatureStateGuard state;
    auto data = make_town_map();
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    const auto before = letter['#'];
    struct InvalidCell {
        nlohmann::json fields;
        ParseErrorType error;
    };
    for (const auto &item : {
             InvalidCell{ { { "terrain", nlohmann::json::object() } }, ParseErrorType::INVALID_TYPE },
             InvalidCell{ { { "special", nlohmann::json::object() } }, ParseErrorType::INVALID_TYPE },
             InvalidCell{ { { "caveInfo", { { "MARK", 1 } } } }, ParseErrorType::INVALID_TYPE },
             InvalidCell{ { { "terrain", nullptr }, { "unknown", 1 } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "terrain", nullptr }, { "monster", nlohmann::json::object() } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "monster", { { "oodLevel", 1 } } } }, ParseErrorType::INVALID_TYPE },
             InvalidCell{ { { "monster", { { "random", false }, { "cloneOf", nullptr } } } }, ParseErrorType::INVALID_TYPE },
             InvalidCell{ { { "monster", { { "random", true }, { "cloneOf", 1 } } } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "trap", "*" } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "terrain", "" } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "caveInfo", { "MARK", "MARK" } } }, ParseErrorType::INVALID_VALUE },
             InvalidCell{ { { "caveInfo", { "UNKNOWN" } } }, ParseErrorType::INVALID_FLAG },
             InvalidCell{ { { "special", 32768 } }, ParseErrorType::INVALID_VALUE },
         }) {
        data["featureRules"][0]["definition"] = item.fields;
        for (const bool only_buildings : { false, true }) {
            CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3, only_buildings)) == item.error);
            CHECK(definition.features.front().terrain == "WALL");
            check_same_grid(letter['#'], before);
        }
    }
    for (const auto &version : std::array<nlohmann::json, 6>{ { 2, 4, 3.0, true, nullptr, "3" } }) {
        data = make_town_map();
        data["version"] = version;
        CHECK(test::to_error_code(TownMapReader(data).read(definition, 0, 0, true)) == ParseErrorType::INVALID_TYPE);
    }
    data = make_town_map();
    data["featureRules"][0]["definition"] = { { "terrain", "UNINITIALIZED_TOWN_TAG" } };
    REQUIRE(TownMapReader(data).read(definition, 0, 0, true).has_value());
    CHECK(definition.features.front().cell.grid.feature == 0);
    CHECK(definition.features.front().cell.grid.trap == 0);
    CHECK(definition.features.front().cell.grid.random == 0);
    data["featureRules"][0]["definition"] = nlohmann::json::object();
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    CHECK(definition.features.front().terrain == "NONE");
    CHECK(definition.features.front().trap == "NONE");
}

// 変換前1b378912b9の各規則の値を固定する。v2 readerや変換関数には依存しない。
static const auto town_v2_expected = nlohmann::json::parse(
    R"townv2({"01_Outpost_Full.jsonc":[["!",null,"FLOOR","NONE",0,0,0,0,3,0,0],["@",null,"FLOOR","NONE",0,0,0,0,3,0,0],["z",null,"TREE","NONE",0,0,0,0,3,0,0],["y",null,"TREE","NONE",0,0,0,0,3,0,0],["x",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["w",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["v",null,"GRASS","NONE",0,0,0,0,3,0,0],["A",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["s",null,"GRASS","NONE",0,0,0,0,3,0,0],["r",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["q",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST1 0]","BUILDING_1","NONE",0,0,0,0,3,1,0],["z","[EQU $QUEST1 1]","QUEST_ENTER","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 1]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 2]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 3]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[EQU $QUEST1 3]","FLOOR","NONE",0,42,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS BeastMaster Cavalry] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS BeastMaster Cavalry] ]","FLOOR","NONE",0,543,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Monk ForceTrainer] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Monk ForceTrainer] ]","FLOOR","NONE",0,126,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Priest] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Priest] ]","FLOOR","NONE",0,55,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Archer] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Archer] ]","FLOOR","NONE",0,73,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Weaponsmith] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Weaponsmith] ]","FLOOR","NONE",0,70,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Ninja] ]","BUILDING_1","NONE",0,0,0,0,3,14,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Ninja] ]","FLOOR","NONE",0,43,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $IRONMAN_DOWNWARD 1] ]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST1 4]","BUILDING_1","NONE",0,0,0,0,3,14,0],["b","[AND [EQU $QUEST1 4] [EQU $IRONMAN_DOWNWARD 1] ]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST1 5]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 6]","BUILDING_1","NONE",0,0,0,0,3,14,0],["b","[AND [EQU $QUEST1 6] [EQU $IRONMAN_DOWNWARD 1] ]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST14 1]","BUILDING_1","NONE",0,0,0,0,3,14,0],["b","[EQU $QUEST14 2]","BUILDING_1","NONE",0,0,0,0,3,14,0],["b","[EQU $QUEST14 3]","BUILDING_1","NONE",0,0,0,0,3,18,0],["!","[EQU $QUEST14 3]","FLOOR","NONE",0,75,0,0,3,0,0],["b","[EQU $QUEST14 4]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST14 6]","BUILDING_1","NONE",0,0,0,0,3,18,0],["y","[EQU $QUEST18 1]","QUEST_ENTER","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST18 1]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST18 2]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST18 3]","BUILDING_1","NONE",0,0,0,0,3,25,0],["!","[EQU $QUEST18 3]","FLOOR","NONE",0,244,0,0,3,0,0],["b","[EQU $QUEST18 4]","BUILDING_1","NONE",0,0,0,0,3,25,0],["b","[EQU $QUEST18 5]","BUILDING_1","NONE",0,0,0,0,3,18,0],["b","[EQU $QUEST18 6]","BUILDING_1","NONE",0,0,0,0,3,25,0],["x","[EQU $QUEST25 1]","QUEST_ENTER","NONE",0,0,0,0,3,25,0],["b","[EQU $QUEST25 1]","BUILDING_1","NONE",0,0,0,0,3,25,0],["b","[EQU $QUEST25 2]","BUILDING_1","NONE",0,0,0,0,3,25,0],["b","[EQU $QUEST25 3]","BUILDING_1","NONE",0,0,0,0,3,28,0],["!","[EQU $QUEST25 3]","FLOOR","NONE",0,587,0,0,3,0,0],["b","[EQU $QUEST25 4]","BUILDING_1","NONE",0,0,0,0,3,28,0],["b","[EQU $QUEST25 5]","BUILDING_1","NONE",0,0,0,0,3,25,0],["b","[EQU $QUEST25 6]","BUILDING_1","NONE",0,0,0,0,3,28,0],["w","[EQU $QUEST28 1]","QUEST_ENTER","NONE",0,0,0,0,3,28,0],["b","[EQU $QUEST28 1]","BUILDING_1","NONE",0,0,0,0,3,28,0],["b","[EQU $QUEST28 2]","BUILDING_1","NONE",0,0,0,0,3,28,0],["b","[EQU $QUEST28 3]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST28 4]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST28 5]","BUILDING_1","NONE",0,0,0,0,3,28,0],["b","[EQU $QUEST28 6]","BUILDING_1","NONE",0,0,0,0,3,0,0],["a","[EQU $QUEST34 0]","BUILDING_0","NONE",0,0,0,0,3,34,0],["A","[EQU $QUEST34 1]","QUEST_ENTER","NONE",0,0,0,0,3,34,0],["a","[EQU $QUEST34 1]","BUILDING_0","NONE",0,0,0,0,3,34,0],["a","[EQU $QUEST34 2]","BUILDING_0","NONE",0,0,0,0,3,34,0],["a","[EQU $QUEST34 3]","BUILDING_0","NONE",0,0,0,0,3,31,0],["@","[EQU $QUEST34 3]","FLOOR","NONE",0,618,0,0,3,0,0],["a","[EQU $QUEST34 4]","BUILDING_0","NONE",0,0,0,0,3,31,0],["a","[EQU $QUEST34 5]","BUILDING_0","NONE",0,0,0,0,3,34,0],["a","[EQU $QUEST34 6]","BUILDING_0","NONE",0,0,0,0,3,31,0],["s","[EQU $QUEST31 1]","QUEST_ENTER","NONE",0,0,0,0,3,31,0],["a","[EQU $QUEST31 1]","BUILDING_0","NONE",0,0,0,0,3,31,0],["a","[EQU $QUEST31 2]","BUILDING_0","NONE",0,0,0,0,3,31,0],["a","[EQU $QUEST31 3]","BUILDING_0","NONE",0,0,0,0,3,32,0],["@","[EQU $QUEST31 3]","FLOOR","NONE",0,430,0,0,3,0,0],["a","[EQU $QUEST31 4]","BUILDING_0","NONE",0,0,0,0,3,32,0],["a","[EQU $QUEST31 5]","BUILDING_0","NONE",0,0,0,0,3,31,0],["a","[EQU $QUEST31 6]","BUILDING_0","NONE",0,0,0,0,3,32,0],["r","[EQU $QUEST32 1]","QUEST_ENTER","NONE",0,0,0,0,3,32,0],["a","[EQU $QUEST32 1]","BUILDING_0","NONE",0,0,0,0,3,32,0],["a","[EQU $QUEST32 2]","BUILDING_0","NONE",0,0,0,0,3,32,0],["a","[EQU $QUEST32 3]","BUILDING_0","NONE",0,0,0,0,3,33,0],["@","[EQU $QUEST32 3]","FLOOR","NONE",0,0,0,1,3,0,0],["a","[EQU $QUEST32 4]","BUILDING_0","NONE",0,0,0,0,3,33,0],["a","[EQU $QUEST32 5]","BUILDING_0","NONE",0,0,0,0,3,32,0],["a","[EQU $QUEST32 6]","BUILDING_0","NONE",0,0,0,0,3,33,0],["q","[EQU $QUEST33 1]","QUEST_ENTER","NONE",0,0,0,0,3,33,0],["a","[EQU $QUEST33 1]","BUILDING_0","NONE",0,0,0,0,3,33,0],["a","[EQU $QUEST33 2]","BUILDING_0","NONE",0,0,0,0,3,33,0],["a","[EQU $QUEST33 3]","BUILDING_0","NONE",0,0,0,0,3,27,0],["@","[EQU $QUEST33 3]","FLOOR","NONE",0,0,0,63,3,0,0],["a","[EQU $QUEST33 4]","BUILDING_0","NONE",0,0,0,0,3,27,0],["a","[EQU $QUEST33 5]","BUILDING_0","NONE",0,0,0,0,3,33,0],["a","[EQU $QUEST33 6]","BUILDING_0","NONE",0,0,0,0,3,27,0],["v","[EQU $QUEST27 1]","QUEST_ENTER","NONE",0,0,0,0,3,27,0],["a","[EQU $QUEST27 1]","BUILDING_0","NONE",0,0,0,0,3,27,0],["a","[EQU $QUEST27 2]","BUILDING_0","NONE",0,0,0,0,3,27,0],["a","[EQU $QUEST27 3]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[EQU $QUEST27 3]","FLOOR","NONE",0,0,0,80,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Weaponsmith] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Weaponsmith] ]","FLOOR","NONE",0,405,0,0,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Tourist] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Tourist] ]","FLOOR","NONE",0,0,0,141,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Ranger Archer] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Ranger Archer] ]","FLOOR","NONE",0,0,0,148,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Warrior Berserker] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Warrior Berserker] ]","FLOOR","NONE",0,0,0,266,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Cavalry] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Cavalry] ]","FLOOR","NONE",0,0,0,255,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Chaos-Warrior] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Chaos-Warrior] ]","FLOOR","NONE",0,0,0,77,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Rogue] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Rogue] ]","FLOOR","NONE",0,0,0,134,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Monk] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Monk] ]","FLOOR","NONE",0,0,0,26,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Ninja] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Ninja] ]","FLOOR","NONE",0,0,0,215,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Mage High-Mage Sorcerer Blue-Mage Mindcrafter] ]","BUILD)townv2"
    R"townv2(ING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Mage High-Mage Sorcerer Blue-Mage Mindcrafter] ]","FLOOR","NONE",0,0,0,33,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Priest ForceTrainer Elementalist] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Priest ForceTrainer Elementalist] ]","FLOOR","NONE",0,0,0,15,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Bard ] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Bard ] ]","FLOOR","NONE",0,0,0,251,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Mirror-Master] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Mirror-Master] ]","FLOOR","NONE",0,0,0,132,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Samurai Warrior-Mage] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Samurai Warrior-Mage] ]","FLOOR","NONE",0,0,0,81,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Red-Mage] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Red-Mage] ]","FLOOR","NONE",0,0,0,252,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Crusade] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Crusade] ]","FLOOR","NONE",0,0,0,217,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Death] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Death] ]","FLOOR","NONE",0,0,0,218,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Magic-Eater BeastMaster] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Magic-Eater BeastMaster] ]","FLOOR","NONE",0,0,0,219,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Imitator] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Imitator] ]","FLOOR","NONE",0,0,0,220,3,0,0],["a","[AND [EQU $QUEST27 3] [EQU $CLASS Sniper] ]","BUILDING_0","NONE",0,0,0,0,3,0,0],["@","[AND [EQU $QUEST27 3] [EQU $CLASS Sniper] ]","FLOOR","NONE",0,0,0,223,3,0,0],["a","[EQU $QUEST27 4]","BUILDING_0","NONE",0,0,0,0,3,0,0],["a","[EQU $QUEST27 5]","BUILDING_0","NONE",0,0,0,0,3,27,0],["a","[EQU $QUEST27 6]","BUILDING_0","NONE",0,0,0,0,3,0,0]],"01_Outpost_Lite.jsonc":[["!",null,"FLOOR","NONE",0,0,0,0,3,0,0],["(",null,"FLOOR","NONE",0,0,0,0,3,0,0],["$",null,"GRASS","NONE",0,0,0,0,3,0,0],["\"",null,"GRASS","NONE",0,0,0,0,3,0,0],["z",null,"TREE","NONE",0,0,0,0,3,0,0],["x",null,"GRASS","NONE",0,0,0,0,3,0,0],["w",null,"GRASS","NONE",0,0,0,0,3,0,0],["v",null,"GRASS","NONE",0,0,0,0,3,0,0],["u",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["t",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["s",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["r",null,"FLOOR","NONE",0,0,0,0,3,0,0],["q",null,"GRASS","NONE",0,0,0,0,3,0,0],[")",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST1 0]","BUILDING_1","NONE",0,0,0,0,3,1,0],["z","[EQU $QUEST1 1]","QUEST_ENTER","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 1]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 2]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 3]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[EQU $QUEST1 3]","FLOOR","NONE",0,42,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS BeastMaster Cavalry] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS BeastMaster Cavalry] ]","FLOOR","NONE",0,543,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Monk ForceTrainer Elementalist] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Monk ForceTrainer Elementalist] ]","FLOOR","NONE",0,126,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Priest] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Priest] ]","FLOOR","NONE",0,55,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Archer] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Archer] ]","FLOOR","NONE",0,73,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Weaponsmith] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Weaponsmith] ]","FLOOR","NONE",0,70,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Mage High-Mage Blue-Mage Sorcerer Mirror-Master] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Mage High-Mage Blue-Mage Sorcerer Mirror-Master] ]","FLOOR","NONE",0,618,0,0,3,0,0],["b","[AND [EQU $QUEST1 3] [EQU $CLASS Ninja] ]","BUILDING_1","NONE",0,0,0,0,3,2,0],["!","[AND [EQU $QUEST1 3] [EQU $CLASS Ninja] ]","FLOOR","NONE",0,43,0,0,3,0,0],["b","[EQU $QUEST1 4]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST1 5]","BUILDING_1","NONE",0,0,0,0,3,1,0],["b","[EQU $QUEST1 6]","BUILDING_1","NONE",0,0,0,0,3,2,0],["r","[EQU $QUEST2 1]","QUEST_ENTER","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 1]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 2]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 3]","BUILDING_1","NONE",0,0,0,0,3,3,0],["!","[EQU $QUEST2 3]","FLOOR","NONE",0,582,0,0,3,0,0],["b","[EQU $QUEST2 4]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST2 5]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 6]","BUILDING_1","NONE",0,0,0,0,3,3,0],["s","[EQU $QUEST3 1]","QUEST_ENTER","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 1]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 2]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 3]","BUILDING_1","NONE",0,0,0,0,3,4,0],["!","[EQU $QUEST3 3]","FLOOR","NONE",0,314,0,0,3,0,0],["b","[EQU $QUEST3 4]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST3 5]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 6]","BUILDING_1","NONE",0,0,0,0,3,4,0],["t","[EQU $QUEST4 1]","QUEST_ENTER","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 1]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 2]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 3]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST4 4]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST4 5]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 6]","BUILDING_1","NONE",0,0,0,0,3,5,0],["u","[EQU $QUEST5 1]","QUEST_ENTER","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 1]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 2]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 3]","BUILDING_1","NONE",0,0,0,0,3,27,0],["!","[EQU $QUEST5 3]","FLOOR","NONE",0,143,0,0,3,0,0],["b","[EQU $QUEST5 4]","BUILDING_1","NONE",0,0,0,0,3,27,0],["b","[EQU $QUEST5 5]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 6]","BUILDING_1","NONE",0,0,0,0,3,27,0],["q","[EQU $QUEST27 1]","QUEST_ENTER","NONE",0,0,0,0,3,27,0],["b","[EQU $QUEST27 1]","BUILDING_1","NONE",0,0,0,0,3,27,0],["b","[EQU $QUEST27 2]","BUILDING_1","NONE",0,0,0,0,3,27,0],["b","[EQU $QUEST27 3]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[EQU $QUEST27 3]","FLOOR","NONE",0,0,0,80,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Weaponsmith] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Weaponsmith] ]","FLOOR","NONE",0,405,0,0,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Tourist] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Tourist] ]","FLOOR","NONE",0,0,0,141,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Ranger Archer] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Ranger Archer] ]","FLOOR","NONE",0,0,0,148,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Warrior Berserker] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Warrior Berserker] ]","FLOOR","NONE",0,0,0,266,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Cavalry] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Cavalry] ]","FLOOR","NONE",0,0,0,163,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Chaos-Warrior] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Chaos-Warrior] ]","FLOOR","NONE",0,0,0,77,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Rogue] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Rogue] ]",")townv2"
    R"townv2(FLOOR","NONE",0,0,0,134,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Monk] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Monk] ]","FLOOR","NONE",0,0,0,26,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Ninja] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Ninja] ]","FLOOR","NONE",0,0,0,215,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Mage High-Mage Sorcerer Blue-Mage Mindcrafter] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Mage High-Mage Sorcerer Blue-Mage Mindcrafter] ]","FLOOR","NONE",0,0,0,33,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Priest Bard ForceTrainer] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Priest Bard ForceTrainer] ]","FLOOR","NONE",0,0,0,15,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Mirror-Master] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Mirror-Master] ]","FLOOR","NONE",0,0,0,132,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Samurai Warrior-Mage] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Samurai Warrior-Mage] ]","FLOOR","NONE",0,0,0,81,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Red-Mage] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Red-Mage] ]","FLOOR","NONE",0,0,0,252,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Crusade] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Crusade] ]","FLOOR","NONE",0,0,0,217,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Death] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Paladin] [EQU $REALM1 Death] ]","FLOOR","NONE",0,0,0,218,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Magic-Eater BeastMaster] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Magic-Eater BeastMaster] ]","FLOOR","NONE",0,0,0,219,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Imitator] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Imitator] ]","FLOOR","NONE",0,0,0,220,3,0,0],["b","[AND [EQU $QUEST27 3] [EQU $CLASS Sniper] ]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[AND [EQU $QUEST27 3] [EQU $CLASS Sniper] ]","FLOOR","NONE",0,0,0,223,3,0,0],["b","[EQU $QUEST27 4]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST27 5]","BUILDING_1","NONE",0,0,0,0,3,27,0],["b","[EQU $QUEST27 6]","BUILDING_1","NONE",0,0,0,0,3,15,0],[")","[EQU $QUEST15 1]","QUEST_ENTER","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 1]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 2]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 3]","BUILDING_1","NONE",0,0,0,0,3,0,0],["!","[EQU $QUEST15 3]","FLOOR","NONE",0,418,0,0,3,0,0],["b","[EQU $QUEST15 4]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST15 5]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 6]","BUILDING_1","NONE",0,0,0,0,3,0,0],["l","[EQU $QUEST10 0]","BUILDING_11","NONE",0,0,0,0,3,10,0],["v","[EQU $QUEST10 1]","QUEST_ENTER","NONE",0,0,0,0,3,10,0],["l","[EQU $QUEST10 1]","BUILDING_11","NONE",0,0,0,0,3,10,0],["l","[EQU $QUEST10 2]","BUILDING_11","NONE",0,0,0,0,3,10,0],["l","[EQU $QUEST10 3]","BUILDING_11","NONE",0,0,0,0,3,0,0],["(","[EQU $QUEST10 3]","FLOOR","NONE",0,337,0,0,3,0,0],["l","[EQU $QUEST10 4]","BUILDING_11","NONE",0,0,0,0,3,0,0],["l","[EQU $QUEST10 5]","BUILDING_11","NONE",0,0,0,0,3,10,0],["l","[EQU $QUEST10 6]","BUILDING_11","NONE",0,0,0,0,3,0,0],["n","[EQU $QUEST11 0]","BUILDING_13","NONE",0,0,0,0,3,11,0],["w","[EQU $QUEST11 1]","QUEST_ENTER","NONE",0,0,0,0,3,11,0],["n","[EQU $QUEST11 1]","BUILDING_13","NONE",0,0,0,0,3,11,0],["n","[EQU $QUEST11 2]","BUILDING_13","NONE",0,0,0,0,3,11,0],["n","[EQU $QUEST11 3]","BUILDING_13","NONE",0,0,0,0,3,0,0],["$","[EQU $QUEST11 3]","GRASS","NONE",0,386,0,0,3,0,0],["n","[EQU $QUEST11 4]","BUILDING_13","NONE",0,0,0,0,3,0,0],["n","[EQU $QUEST11 5]","BUILDING_13","NONE",0,0,0,0,3,11,0],["n","[EQU $QUEST11 6]","BUILDING_13","NONE",0,0,0,0,3,0,0],["o","[EQU $QUEST12 0]","BUILDING_14","NONE",0,0,0,0,3,12,0],["x","[EQU $QUEST12 1]","QUEST_ENTER","NONE",0,0,0,0,3,12,0],["o","[EQU $QUEST12 1]","BUILDING_14","NONE",0,0,0,0,3,12,0],["o","[EQU $QUEST12 2]","BUILDING_14","NONE",0,0,0,0,3,12,0],["o","[EQU $QUEST12 3]","BUILDING_14","NONE",0,0,0,0,3,0,0],["\"","[EQU $QUEST12 3]","GRASS","NONE",0,382,0,0,3,0,0],["o","[EQU $QUEST12 4]","BUILDING_14","NONE",0,0,0,0,3,0,0],["o","[EQU $QUEST12 5]","BUILDING_14","NONE",0,0,0,0,3,12,0],["o","[EQU $QUEST12 6]","BUILDING_14","NONE",0,0,0,0,3,0,0]],"01_Outpost_OnlyAngband.jsonc":[],"02_Telmora.jsonc":[["!",null,"FLOOR","NONE",0,0,0,0,3,0,0],["z",null,"FLOOR","NONE",0,0,0,0,3,0,0],["y",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["x",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["w",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["v",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["u",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["t",null,"TREE","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST2 0]","BUILDING_1","NONE",0,0,0,0,3,2,0],["z","[EQU $QUEST2 1]","QUEST_ENTER","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 1]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 2]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 3]","BUILDING_1","NONE",0,0,0,0,3,3,0],["!","[EQU $QUEST2 3]","FLOOR","NONE",0,582,0,0,3,0,0],["b","[EQU $QUEST2 4]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST2 5]","BUILDING_1","NONE",0,0,0,0,3,2,0],["b","[EQU $QUEST2 6]","BUILDING_1","NONE",0,0,0,0,3,3,0],["y","[EQU $QUEST3 1]","QUEST_ENTER","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 1]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 2]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 3]","BUILDING_1","NONE",0,0,0,0,3,4,0],["!","[EQU $QUEST3 3]","FLOOR","NONE",0,314,0,0,3,0,0],["b","[EQU $QUEST3 4]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST3 5]","BUILDING_1","NONE",0,0,0,0,3,3,0],["b","[EQU $QUEST3 6]","BUILDING_1","NONE",0,0,0,0,3,4,0],["x","[EQU $QUEST4 1]","QUEST_ENTER","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 1]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 2]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 3]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST4 4]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST4 5]","BUILDING_1","NONE",0,0,0,0,3,4,0],["b","[EQU $QUEST4 6]","BUILDING_1","NONE",0,0,0,0,3,5,0],["w","[EQU $QUEST5 1]","QUEST_ENTER","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 1]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 2]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 3]","BUILDING_1","NONE",0,0,0,0,3,26,0],["!","[EQU $QUEST5 3]","FLOOR","NONE",0,143,0,0,3,0,0],["b","[EQU $QUEST5 4]","BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST5 5]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[EQU $QUEST5 6]","BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[AND [EQU $QUEST5 3] [AND [EQU $CLASS Paladin] [EQU $REALM1 Death] ] ]","BUILDING_1","NONE",0,0,0,0,3,16,0],["!","[AND [EQU $QUEST5 3] [AND [EQU $CLASS Paladin] [EQU $REALM1 Death] ] ]","FLOOR","NONE",0,143,0,0,3,0,0],["b","[AND [EQU $QUEST5 4] [AND [EQU $CLASS Paladin] [EQU $REALM1 Death] ] ]","BUILDING_1","NONE",0,0,0,0,3,16,0],["b","[AND [EQU $QUEST5 5] [AND [EQU $CLASS Paladin] [EQU $REALM1 Death] ] ]","BUILDING_1","NONE",0,0,0,0,3,5,0],["b","[AND [EQU $QUEST5 6] [AND [EQU $CLASS Paladin] [EQU $REALM1 Death] ] ]","BUILDING_1","NONE",0,0,0,0,3,16,0],["u","[EQU $QUEST16 1]","QUEST_ENTER","NONE",0,0,0,0,3,16,0],["b","[EQU $QUEST16 1]","BUILDING_1","NONE",0,0,0,0,3,16,0],["b","[EQU $QUEST16 2]","BUILDING_1","NONE",0,0,0,0,3,16,0],["b","[EQU $QUEST16 3]","BUILDING_1","NONE",0,0,0,0,3,26,0],["!","[EQU $QUEST16 3]","FLOOR","NONE",0,520,0,0,3,0,0],["b","[EQU $QUEST16 4]","BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST16 5]","BUILDING_1","NONE",0,0,0,0,3,16,0],["b","[EQU $QUEST16 6]","BUILDING_1","NONE",0,0,0,0,3,26,0],["t","[EQU $QUEST26 1]","QUEST_ENTER","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST26 1]",)townv2"
    R"townv2("BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST26 2]","BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST26 3]","BUILDING_1","NONE",0,0,0,0,3,15,0],["!","[EQU $QUEST26 3]","FLOOR","NONE",0,307,0,0,3,0,0],["b","[EQU $QUEST26 4]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST26 5]","BUILDING_1","NONE",0,0,0,0,3,26,0],["b","[EQU $QUEST26 6]","BUILDING_1","NONE",0,0,0,0,3,15,0],["v","[EQU $QUEST15 1]","QUEST_ENTER","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 1]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 2]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 3]","BUILDING_1","NONE",0,0,0,0,3,0,0],["!","[EQU $QUEST15 3]","FLOOR","NONE",0,418,0,0,3,0,0],["b","[EQU $QUEST15 4]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST15 5]","BUILDING_1","NONE",0,0,0,0,3,15,0],["b","[EQU $QUEST15 6]","BUILDING_1","NONE",0,0,0,0,3,0,0]],"03_Morivant.jsonc":[["~",null,"FLOOR","NONE",0,0,0,0,3,0,0],["@",null,"FLOOR","NONE",0,0,0,0,3,0,0],["z",null,"GRASS","NONE",0,0,0,0,3,0,0],["y",null,"GRASS","NONE",0,0,0,0,3,0,0],["x",null,"GRASS","NONE",0,0,0,0,3,0,0],["v",null,"FLOOR","NONE",0,0,0,0,3,0,0],["w",null,"GRASS","NONE",0,0,0,0,3,0,0],["u",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["s",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["r",null,"PERMANENT","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST20 0]","BUILDING_1","NONE",0,0,0,0,3,20,0],["v","[EQU $QUEST20 1]","QUEST_ENTER","NONE",0,0,0,0,3,20,0],["b","[EQU $QUEST20 1]","BUILDING_1","NONE",0,0,0,0,3,20,0],["b","[EQU $QUEST20 2]","BUILDING_1","NONE",0,0,0,0,3,20,0],["b","[EQU $QUEST20 3]","BUILDING_1","NONE",0,0,0,0,3,24,0],["~","[EQU $QUEST20 3]","FLOOR","NONE",0,375,0,0,3,0,0],["b","[EQU $QUEST20 4]","BUILDING_1","NONE",0,0,0,0,3,24,0],["b","[EQU $QUEST20 5]","BUILDING_1","NONE",0,0,0,0,3,20,0],["b","[EQU $QUEST20 6]","BUILDING_1","NONE",0,0,0,0,3,24,0],["u","[EQU $QUEST24 1]","QUEST_ENTER","NONE",0,0,0,0,3,24,0],["b","[EQU $QUEST24 1]","BUILDING_1","NONE",0,0,0,0,3,24,0],["b","[EQU $QUEST24 2]","BUILDING_1","NONE",0,0,0,0,3,24,0],["~","[EQU $QUEST24 3]","FLOOR","NONE",0,432,0,0,3,0,0],["b","[AND [EQU $QUEST24 3 4] [GEQ $QUEST19 4] ]","BUILDING_1","NONE",0,0,0,0,3,21,0],["b","[AND [EQU $QUEST24 3 4] [LEQ $QUEST19 3] ]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST24 5]","BUILDING_1","NONE",0,0,0,0,3,24,0],["b","[AND [EQU $QUEST24 6] [GEQ $QUEST19 4] ]","BUILDING_1","NONE",0,0,0,0,3,21,0],["b","[AND [EQU $QUEST24 6] [LEQ $QUEST19 3] ]","BUILDING_1","NONE",0,0,0,0,3,0,0],["w","[EQU $QUEST21 1]","QUEST_ENTER","NONE",0,0,0,0,3,21,0],["b","[EQU $QUEST21 1]","BUILDING_1","NONE",0,0,0,0,3,21,0],["b","[EQU $QUEST21 2]","BUILDING_1","NONE",0,0,0,0,3,21,0],["b","[EQU $QUEST21 3]","BUILDING_1","NONE",0,0,0,0,3,0,0],["~","[EQU $QUEST21 3]","FLOOR","NONE",0,329,0,0,3,0,0],["b","[EQU $QUEST21 4]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST21 5]","BUILDING_1","NONE",0,0,0,0,3,21,0],["b","[EQU $QUEST21 6]","BUILDING_1","NONE",0,0,0,0,3,0,0],["k","[EQU $QUEST29 0]","BUILDING_10","NONE",0,0,0,0,3,29,0],["s","[EQU $QUEST29 1]","QUEST_ENTER","NONE",0,0,0,0,3,29,0],["k","[EQU $QUEST29 1]","BUILDING_10","NONE",0,0,0,0,3,29,0],["k","[EQU $QUEST29 2]","BUILDING_10","NONE",0,0,0,0,3,29,0],["k","[EQU $QUEST29 3]","BUILDING_10","NONE",0,0,0,0,3,30,0],["@","[EQU $QUEST29 3]","FLOOR","NONE",0,155,0,0,3,0,0],["k","[EQU $QUEST29 4]","BUILDING_10","NONE",0,0,0,0,3,30,0],["k","[EQU $QUEST29 5]","BUILDING_10","NONE",0,0,0,0,3,29,0],["k","[EQU $QUEST29 6]","BUILDING_10","NONE",0,0,0,0,3,30,0],["r","[EQU $QUEST30 1]","QUEST_ENTER","NONE",0,0,0,0,3,30,0],["k","[EQU $QUEST30 1]","BUILDING_10","NONE",0,0,0,0,3,30,0],["k","[EQU $QUEST30 2]","BUILDING_10","NONE",0,0,0,0,3,30,0],["k","[EQU $QUEST30 3]","BUILDING_10","NONE",0,0,0,0,3,0,0],["@","[EQU $QUEST30 3]","FLOOR","NONE",0,144,0,0,3,0,0],["k","[EQU $QUEST30 4]","BUILDING_10","NONE",0,0,0,0,3,0,0],["k","[EQU $QUEST30 5]","BUILDING_10","NONE",0,0,0,0,3,30,0],["k","[EQU $QUEST30 6]","BUILDING_10","NONE",0,0,0,0,3,0,0]],"04_Angwil.jsonc":[["!",null,"FLOOR","NONE",0,0,0,0,3,0,0],["z",null,"TREE","NONE",0,0,0,0,3,0,0],["y",null,"TREE","NONE",0,0,0,0,3,0,0],["x",null,"TREE","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST22 0]","BUILDING_1","NONE",0,0,0,0,3,22,0],["y","[EQU $QUEST22 1]","QUEST_ENTER","NONE",0,0,0,0,3,22,0],["b","[EQU $QUEST22 1]","BUILDING_1","NONE",0,0,0,0,3,22,0],["b","[EQU $QUEST22 2]","BUILDING_1","NONE",0,0,0,0,3,22,0],["b","[EQU $QUEST22 3]","BUILDING_1","NONE",0,0,0,0,3,19,0],["!","[EQU $QUEST22 3]","FLOOR","NONE",0,90,0,0,3,0,0],["b","[EQU $QUEST22 4]","BUILDING_1","NONE",0,0,0,0,3,19,0],["b","[EQU $QUEST22 5]","BUILDING_1","NONE",0,0,0,0,3,22,0],["b","[EQU $QUEST22 6]","BUILDING_1","NONE",0,0,0,0,3,19,0],["z","[EQU $QUEST19 1]","QUEST_ENTER","NONE",0,0,0,0,3,19,0],["b","[EQU $QUEST19 1]","BUILDING_1","NONE",0,0,0,0,3,19,0],["b","[EQU $QUEST19 2]","BUILDING_1","NONE",0,0,0,0,3,19,0],["b","[EQU $QUEST19 3]","BUILDING_1","NONE",0,0,0,0,3,23,0],["!","[EQU $QUEST19 3]","FLOOR","NONE",0,146,0,0,3,0,0],["b","[EQU $QUEST19 4]","BUILDING_1","NONE",0,0,0,0,3,23,0],["b","[EQU $QUEST19 5]","BUILDING_1","NONE",0,0,0,0,3,19,0],["b","[EQU $QUEST19 6]","BUILDING_1","NONE",0,0,0,0,3,23,0],["x","[EQU $QUEST23 1]","QUEST_ENTER","NONE",0,0,0,0,3,23,0],["b","[EQU $QUEST23 1]","BUILDING_1","NONE",0,0,0,0,3,23,0],["b","[EQU $QUEST23 2]","BUILDING_1","NONE",0,0,0,0,3,23,0],["b","[EQU $QUEST23 3]","BUILDING_1","NONE",0,0,0,0,3,0,0],["!","[EQU $QUEST23 3]","FLOOR","NONE",0,411,0,0,3,0,0],["b","[EQU $QUEST23 4]","BUILDING_1","NONE",0,0,0,0,3,0,0],["b","[EQU $QUEST23 5]","BUILDING_1","NONE",0,0,0,0,3,23,0],["b","[EQU $QUEST23 6]","BUILDING_1","NONE",0,0,0,0,3,0,0]],"05_Zul.jsonc":[["!",null,"FLOOR","NONE",0,0,0,0,3,0,0],["$",null,"FLOOR","NONE",0,0,0,0,3,0,0],["\"",null,"FLOOR","NONE",0,0,0,0,3,0,0],["%",null,"FLOOR","NONE",0,0,0,0,3,0,0],["z",null,"SHALLOW_WATER","NONE",0,0,0,0,3,0,0],["y",null,"MOUNTAIN","NONE",0,0,0,0,3,0,0],["x",null,"TREE","NONE",0,0,0,0,3,0,0],["w",null,"MOUNTAIN","NONE",0,0,0,0,3,0,0],["i","[EQU $QUEST10 0]","BUILDING_8","NONE",0,0,0,0,3,10,0],["z","[EQU $QUEST10 1]","QUEST_ENTER","NONE",0,0,0,0,3,10,0],["i","[EQU $QUEST10 1]","BUILDING_8","NONE",0,0,0,0,3,10,0],["i","[EQU $QUEST10 2]","BUILDING_8","NONE",0,0,0,0,3,10,0],["i","[EQU $QUEST10 3]","BUILDING_8","NONE",0,0,0,0,3,0,0],["!","[EQU $QUEST10 3]","FLOOR","NONE",0,337,0,0,3,0,0],["i","[EQU $QUEST10 4]","BUILDING_8","NONE",0,0,0,0,3,0,0],["i","[EQU $QUEST10 5]","BUILDING_8","NONE",0,0,0,0,3,10,0],["i","[EQU $QUEST10 6]","BUILDING_8","NONE",0,0,0,0,3,0,0],["o","[EQU $QUEST11 0]","BUILDING_14","NONE",0,0,0,0,3,11,0],["y","[EQU $QUEST11 1]","QUEST_ENTER","NONE",0,0,0,0,3,11,0],["o","[EQU $QUEST11 1]","BUILDING_14","NONE",0,0,0,0,3,11,0],["o","[EQU $QUEST11 2]","BUILDING_14","NONE",0,0,0,0,3,11,0],["o","[EQU $QUEST11 3]","BUILDING_14","NONE",0,0,0,0,3,0,0],["$","[EQU $QUEST11 3]","GRASS","NONE",0,386,0,0,3,0,0],["o","[EQU $QUEST11 4]","BUILDING_14","NONE",0,0,0,0,3,0,0],["o","[EQU $QUEST11 5]","BUILDING_14","NONE",0,0,0,0,3,11,0],["o","[EQU $QUEST11 6]","BUILDING_14","NONE",0,0,0,0,3,0,0],["p","[EQU $QUEST12 0]","BUILDING_15","NONE",0,0,0,0,3,12,0],["x","[EQU $QUEST12 1]","QUEST_ENTER","NONE",0,0,0,0,3,12,0],["p","[EQU $QUEST12 1]","BUILDING_15","NONE",0,0,0,0,3,12,0],["p","[EQU $QUEST12 2]","BUILDING_15","NONE",0,0,0,0,3,12,0],["p","[EQU $QUEST12 3]","BUILDING_15","NONE",0,0,0,0,3,0,0],["\"","[EQU $QUEST12 3]","GRASS","NONE",0,382,0,0,3,0,0],["p","[EQU $QUEST12 4]","BUILDING_15","NONE",0,0,0,0,3,0,0],["p","[EQU $QUEST12 5]","BUILDING_15","NONE",0,0,0,0,3,12,0],["p","[EQU $QUEST12 6]","BUILDING_15","NONE",0,0,0,0,3,0,0],["r","[EQU $QUEST13 0]","BUILDING_16","NONE",0,0,0,0,3,13,0],["w","[EQU $QUEST13 1]","QUEST_ENTER","NONE",0,0,0,0,3,13,0],["r","[EQU $QUEST13 1]","BUILDING_16","NONE",0,0,0,0,3,13,0],["r","[EQU $QUEST13 2]","BUILDING_16","NONE",0,0,0,0,3,13,0],["r","[EQU $QUEST13 3]","BUILDING_16","NONE",0,0,0,0,3,0,0],["%","[EQU $QUEST13 3]","GRASS","NONE",0,519,0,0,3,0,0],["r","[EQU $QUEST13 4]","BUILDING_16",)townv2"
    R"townv2("NONE",0,0,0,0,3,0,0],["r","[EQU $QUEST13 5]","BUILDING_16","NONE",0,0,0,0,3,13,0],["r","[EQU $QUEST13 6]","BUILDING_16","NONE",0,0,0,0,3,0,0]],"06_Rlyeh_Obsoleted.jsonc":[["e",null,"DEEP_WATER","NONE",0,0,0,0,8,0,0],["f",null,"SHALLOW_WATER","NONE",0,0,0,0,8,0,0],[".","[EQU $QUEST17 1]","FLOOR","NONE",0,0,0,0,3,0,0],["#","[EQU $QUEST17 1]","GRANITE","NONE",0,0,0,0,12,0,0],["-","[EQU $QUEST17 1]","FLOOR","NONE",0,0,0,0,12,0,0],["x","[EQU $QUEST17 1]","FLOOR","NONE",0,424,0,0,12,0,0],["c","[EQU $QUEST17 1]","FLOOR","NONE",836,0,0,0,8,0,0],["C","[EQU $QUEST17 1]","FLOOR","*",857,0,0,0,8,0,32],["*","[EQU $QUEST17 1]","FLOOR","*",0,0,0,0,12,0,36],["^","[EQU $QUEST17 1]","FLOOR","*",0,0,0,0,12,0,32],["$","[EQU $QUEST17 1]","FLOOR","*",0,10,0,0,12,0,36],["!","[EQU $QUEST17 1]","FLOOR","*",0,25,0,0,12,0,36],["D","[EQU $QUEST17 1]","CLOSED_DOOR","NONE",0,0,0,0,12,0,0],["%","[EQU $QUEST17 1]","GRANITE","NONE",0,0,0,0,12,0,0]]})townv2");

TEST_CASE("TownMapReader bundled v3 cells preserve all v2 letter values")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper terrain_state(terrains);
    std::map<std::string, short> tags;
    for (const auto &rows : town_v2_expected) {
        for (const auto &row : rows) {
            for (const size_t column : { size_t{ 2 }, size_t{ 3 } }) {
                const auto tag = row[column].get<std::string>();
                if (tag == "*" || tags.contains(tag)) {
                    continue;
                }
                const auto id = static_cast<short>(terrains.size());
                terrains.resize(terrains.size() + 1);
                terrains.get_terrain(id).tag = tag;
                tags.emplace(tag, id);
            }
        }
    }
    // NONEを非ゼロにして、ランダム指定もNONEのIDで初期化されることを確認する。
    REQUIRE(tags.at("NONE") != 0);
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, tags.at("NONE"));
    // autotoolsは実行時の環境変数でVPATHのソース位置を明示する。
    std::filesystem::path root;
    if (const auto *source_dir = std::getenv("HENGBAND_TEST_SOURCE_DIR"); source_dir && *source_dir) {
        root = source_dir;
    } else {
        // MSBuildの相対__FILE__は実行時cwdに依存するため、まずcwdの祖先を探す。
        // 環境変数も絶対__FILE__もない場合、別checkoutからの実行は識別できない。
        for (auto candidate : { std::filesystem::current_path(), std::filesystem::absolute(__FILE__).parent_path() }) {
            while (!candidate.empty()) {
                if (std::filesystem::exists(candidate / "src/test/info-reader/test-town-map-reader.cpp") &&
                    std::filesystem::exists(candidate / "lib/edit/towns/01_Outpost_Full.jsonc")) {
                    root = candidate;
                    break;
                }
                const auto parent = candidate.parent_path();
                if (parent == candidate) {
                    break;
                }
                candidate = parent;
            }
            if (!root.empty()) {
                break;
            }
        }
    }
    REQUIRE_FALSE(root.empty());
    CAPTURE(root);

    for (const auto &[name, expected_rows] : town_v2_expected.items()) {
        CAPTURE(name);
        JsoncDocumentLoader loader(root / "lib/edit/towns" / name);
        REQUIRE(loader.is_open());
        const auto data = loader.parse();
        // ビルド後にソースcheckoutが戻された場合も、実際に読んだ版を診断に残す。
        const auto input_version = data.value("version", nlohmann::json{}).dump();
        CAPTURE(input_version);
        REQUIRE_MESSAGE(data.value("version", nlohmann::json{}) == 3, "The test binary and bundled town data must come from the same source revision.");
        TownMapDefinition definition;
        const auto read_result = TownMapReader(data).read(definition, 66, 198);
        CAPTURE(test::to_error_code(read_result));
        if (!read_result) {
            // read()は規則位置を返さないため、有効な最小文書で各規則を独立に診断する。
            for (const auto *section : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
                CAPTURE(std::string(section));
                for (size_t i = 0; i < data.at(section).size(); ++i) {
                    CAPTURE(i);
                    const auto rule = data.at(section)[i].dump();
                    CAPTURE(rule);
                    auto isolated = make_town_map();
                    if (std::string_view(section) == "startingPositions") {
                        isolated["mapVariants"] = data.at("mapVariants");
                    }
                    isolated[section] = nlohmann::json::array({ data.at(section)[i] });
                    TownMapDefinition diagnostic;
                    const auto result = TownMapReader(isolated).read(diagnostic, 66, 198);
                    CAPTURE(test::to_error_code(result));
                    CHECK(result.has_value());
                }
            }
        }
        REQUIRE(read_result.has_value());
        REQUIRE(definition.features.size() == expected_rows.size());
        for (size_t i = 0; i < expected_rows.size(); ++i) {
            CAPTURE(i);
            const auto &row = expected_rows[i];
            const auto &feature = definition.features[i];
            CHECK(std::string(1, feature.symbol) == row[0].get<std::string>());
            CHECK(feature.condition == (row[1].is_null() ? std::optional<std::string>{} : std::optional<std::string>{ row[1].get<std::string>() }));
            CHECK(feature.terrain == row[2].get<std::string>());
            CHECK(feature.trap == row[3].get<std::string>());
            CHECK(feature.cell.grid.feature == 0);
            CHECK(feature.cell.grid.trap == 0);
            CHECK((feature.cell.grid.random & (RANDOM_FEATURE | RANDOM_TRAP)) == 0);
            dungeon_grid expected{};
            const auto terrain_tag = row[2].get<std::string>();
            const auto trap_tag = row[3].get<std::string>();
            expected.feature = tags.at(terrain_tag == "*" ? "NONE" : terrain_tag);
            expected.trap = tags.at(trap_tag == "*" ? "NONE" : trap_tag);
            expected.monster = row[4].get<short>();
            expected.object = row[5].get<short>();
            expected.ego = i2enum<EgoType>(row[6].get<int>());
            expected.artifact = i2enum<FixedArtifactId>(row[7].get<int>());
            expected.cave_info = row[8].get<BIT_FLAGS>();
            expected.special = row[9].get<int16_t>();
            expected.random = row[10].get<int>();
            // 全規則を単独で適用し、条件の成否によらず固定期待値と比較する。
            REQUIRE(apply_town_map_feature(floor, feature).has_value());
            check_same_grid(fixed_map_letter_at(static_cast<unsigned char>(feature.symbol)), expected);
        }
        // 各ファイルの異なる条件式を固定コンテキストで一度ずつ評価し、解析エラーを検出する。
        std::set<std::string> conditions;
        for (const auto *section : { "featureRules", "buildingRules", "mapVariants", "startingPositions" }) {
            for (const auto &rule : data.at(section)) {
                if (rule.contains("when")) {
                    conditions.insert(rule.at("when").get<std::string>());
                }
            }
        }
        const ExpressionVariableResolver resolver = [](std::string_view variable) -> ExpressionVariableResult {
            if (variable == "CLASS") {
                return tl::optional<std::string>{ "Warrior" };
            }
            if (variable == "REALM1") {
                return tl::optional<std::string>{ "Life" };
            }
            if (variable == "RACE") {
                return tl::optional<std::string>{ "Human" };
            }
            if (variable == "IRONMAN_DOWNWARD" || variable == "LEAVING_QUEST" || variable.starts_with("QUEST")) {
                return tl::optional<std::string>{ "0" };
            }
            return tl::optional<std::string>{};
        };
        for (const auto &condition : conditions) {
            CAPTURE(condition);
            CHECK(evaluate_condition_expression_checked(condition, resolver).has_value());
        }
    }
}

TEST_CASE("TownMapReader later v3 rules replace the whole cell")
{
    TownFeatureStateGuard state;
    FloorType floor;
    floor.quest_number = QuestId::NONE;
    auto data = make_town_map();
    data["featureRules"][0]["definition"] = {
        { "terrain", "*" }, { "trap", { { "random", true } } },
        { "monster", { { "random", true }, { "oodLevel", 10 } } },
        { "object", { { "random", true }, { "oodLevel", 25 } } },
        { "artifact", { { "random", true }, { "id", 3 } } },
        { "caveInfo", { "MARK", "GLOW" } }, { "special", 42 }
    };
    data["featureRules"].push_back({ { "symbol", "#" }, { "definition", { { "terrain", "TOWN_FEATURE_TEST_FLOOR" }, { "trap", "TOWN_FEATURE_TEST_TRAP" } } } });
    TownMapDefinition definition;
    REQUIRE(TownMapReader(data).read(definition, 3, 3).has_value());
    REQUIRE(apply_town_map_feature(floor, definition.features[0]).has_value());
    CHECK(letter['#'].object == 25);
    CHECK(letter['#'].random == (RANDOM_FEATURE | RANDOM_MONSTER | RANDOM_OBJECT | RANDOM_ARTIFACT | RANDOM_TRAP));
    REQUIRE(apply_town_map_feature(floor, definition.features[1]).has_value());
    dungeon_grid expected{};
    expected.feature = state.floor_id();
    expected.trap = state.trap_id();
    check_same_grid(letter['#'], expected);
}

TEST_CASE("TownMapReader reports incompatible ego with rule and symbol")
{
    const test::ScopedReaderState reader_state;
    const test::LegendEgoFixture ego_state;
    auto data = make_town_map();
    data["featureRules"][0]["definition"]["object"] = 54;
    data["featureRules"][0]["definition"]["ego"] = static_cast<int>(EgoType::DF);
    for (const bool only_buildings : { false, true }) {
        ScopedMessageCapture diagnostics;
        TownMapDefinition definition;
        CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3, only_buildings)) == ParseErrorType::INVALID_VALUE);
        CHECK(definition.features.empty());
        REQUIRE(diagnostics.messages().size() == 1);
        CHECK(diagnostics.messages().front() == _("町のエゴ指定が不正です: featureRules[0] (記号 '#').ego 65 (object 54): 品の種類と相性が合わない",
                                                    "Invalid town ego: featureRules[0] (symbol '#').ego 65 (object 54): incompatible object kind"));
    }
}

TEST_CASE("TownMapReader reports fixed trap tags including typos in conflicts")
{
    const test::ScopedReaderState reader_state;
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper<TerrainList> terrain_state(terrains);
    terrains.resize(2);
    terrains.get_terrain(0).tag = "NONE";
    terrains.get_terrain(1).tag = "TRAP_PIT";
    for (const auto *tag : { "TRAP_PIT", "TRAP_PTI" }) {
        for (const bool only_buildings : { false, true }) {
            CAPTURE(tag);
            CAPTURE(only_buildings);
            auto data = make_town_map();
            auto &fields = data["featureRules"][0]["definition"];
            fields["object"] = 54;
            fields["trap"] = tag;
            ScopedMessageCapture diagnostics;
            TownMapDefinition definition;
            CHECK(test::to_error_code(TownMapReader(data).read(definition, 3, 3, only_buildings)) == ParseErrorType::INVALID_VALUE);
            CHECK(definition.features.empty());
            REQUIRE(diagnostics.messages().size() == 1);
            const auto expected = fmt::format(_("町の配置指定が不正です: featureRules[0] (記号 '#').trap: {} (trap '{}' は固定の罠として扱われます。未知のタグや誤記も固定の罠になります)",
                                                  "Invalid town placement: featureRules[0] (symbol '#').trap: {} (trap '{}' is treated as a fixed trap; unknown tags and typos are treated the same way)"),
                describe_legend_object_trap_conflict(), tag);
            CHECK(diagnostics.messages().front() == expected);
        }
    }
}

TEST_CASE("TownMapReader reports oversize maps as out of bounds")
{
    for (const auto &[height, width] : { std::pair{ 2, 3 }, std::pair{ 3, 2 }, std::pair{ 0, 3 }, std::pair{ 3, 0 } }) {
        const auto data = make_town_map();
        TownMapDefinition definition;
        CHECK(test::to_error_code(TownMapReader(data).read(definition, height, width)) == ParseErrorType::OUT_OF_BOUNDS);
    }
    auto data = make_town_map();
    data["mapVariants"][0]["rows"][1] = "####";
    CHECK(test::to_error_code(read_town(data)) == ParseErrorType::OUT_OF_BOUNDS);
}
