#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/quest-reader.h"
#include "info-reader/town-map-reader.h"
#include "info-reader/town-preferences-reader.h"
#include "locale/language-switcher.h"
#include "system/dungeon/quest-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/terrain-list-test-access.h"
#include "view/display-messages.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <string>

TEST_CASE("Legend object and trap placement conflicts are rejected in every reader")
{
    const test::ScopedReaderState reader_state;
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper<TerrainList> terrain_state(terrains);
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    terrains.resize(2);
    terrains.get_terrain(0).tag = "NONE";
    terrains.get_terrain(1).tag = "TRAP_PIT";
    const auto cases = nlohmann::json::parse(R"([
        [{"object":1,"trap":"TRAP_PIT"},false],
        [{"object":42,"ego":65,"trap":"TRAP_PIT"},false],
        [{"object":42,"ego":65,"trap":{"random":true}},false],
        [{"object":42,"ego":{"random":true},"trap":"TRAP_PIT"},false],
        [{"object":42,"ego":{"random":true},"trap":{"random":true}},false],
        [{"object":{"questReward":true},"trap":"TRAP_PIT"},false],
        [{"object":{"questReward":true},"trap":{"random":true}},false],
        [{"object":{"questReward":true},"trap":"NONE"},true],
        [{"object":{"random":true},"trap":"TRAP_PIT"},false],
        [{"object":1,"trap":{"random":true}},false],
        [{"object":{"random":true},"trap":{"random":true}},true],
        [{"object":1,"trap":"NONE"},true],
        [{"artifact":1,"trap":"TRAP_PIT"},true],
        [{"object":{"random":true},"trap":"NONE"},true],
        [{"artifact":1,"trap":{"random":true}},true]
    ])");
    for (const auto &entry : cases) {
        CAPTURE(entry.dump());
        auto cell = entry[0];
        cell["terrain"] = "NONE";
        cell["caveInfo"] = nlohmann::json::array();
        const auto expected = entry[1].get<bool>() ? ParseErrorType::NONE : ParseErrorType::INVALID_VALUE;
        QuestLegendCell immediate;
        CHECK(test::to_error_code(parse_quest_legend_cell(cell, immediate)) == expected);
        DeferredQuestLegendCell deferred;
        deferred.terrain_tag = "Previous";
        CHECK(test::to_error_code(parse_quest_legend_cell_deferred(cell, deferred)) == expected);
        if (expected != ParseErrorType::NONE) {
            CHECK(deferred.terrain_tag == "Previous");
        }

        auto quest_data = nlohmann::json::parse(R"({"name":{"ja":"Test","en":"Test"},"definition":{"type":"KILL_ALL","level":1},"legend":{}})");
        quest_data["legend"]["#"] = cell;
        QuestType quest;
        quest.name = "Previous";
        QuestFixedMap map;
        std::string reason;
        CHECK(test::to_error_code(QuestReader(quest_data, quest, map).read(&reason)) == expected);
        if (expected != ParseErrorType::NONE) {
            CHECK(quest.name == "Previous");
            CHECK(reason.find("legend['#'].trap") != std::string::npos);
            CHECK(reason.find(describe_legend_object_trap_conflict()) != std::string::npos);
        }

        auto preferences = nlohmann::json::parse(R"({"version":1,"legend":{}})");
        preferences["legend"]["#"] = cell;
        TownPreferencesLegend legend;
        CHECK(test::to_error_code(TownPreferencesReader(preferences).read(legend, parse_quest_legend_cell)) == expected);

        auto town = nlohmann::json::parse(R"({"version":3,"featureRules":[{"symbol":"#","when":"0","definition":{}}],"buildingRules":[],"mapVariants":[{"rows":["#"]}],"startingPositions":[{"y":0,"x":0}]})");
        town["featureRules"][0]["definition"] = cell;
        for (const bool only_buildings : { false, true }) {
            CAPTURE(only_buildings);
            TownMapDefinition definition;
            CHECK(test::to_error_code(TownMapReader(town).read(definition, 1, 1, only_buildings)) == expected);
        }
    }
}

TEST_CASE("Legend parse failures do not borrow object trap conflict reasons")
{
    const test::ScopedReaderState reader_state;
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper<TerrainList> terrain_state(terrains);
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    terrains.resize(2);
    terrains.get_terrain(0).tag = "NONE";
    terrains.get_terrain(1).tag = "OTHER";
    const auto cases = nlohmann::json::parse(R"([
        [{"object":1,"trap":"OTHER","special":"x"},false],
        [{"object":1,"trap":"OTHER","special":32768},true]
    ])");
    for (const auto &entry : cases) {
        CAPTURE(entry.dump());
        const auto expected = entry[1].get<bool>() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE;
        QuestLegendCell cell;
        std::string reason = "Previous";
        CHECK(test::to_error_code(parse_quest_legend_cell(entry[0], cell, reason)) == expected);
        CHECK(reason.empty());
        DeferredQuestLegendCell deferred;
        CHECK(test::to_error_code(parse_quest_legend_cell_deferred(entry[0], deferred)) == expected);
        auto data = nlohmann::json::parse(R"({"name":{"ja":"Test","en":"Test"},"definition":{"type":"KILL_ALL","level":1},"legend":{}})");
        data["legend"]["#"] = entry[0];
        QuestType quest;
        QuestFixedMap map;
        reason = "Previous";
        CHECK(test::to_error_code(QuestReader(data, quest, map).read(&reason)) == expected);
        CHECK(reason.empty());
    }
}

TEST_CASE("TownPreferencesReader checks conflicts from an injected cell parser")
{
    const auto data = nlohmann::json::parse(R"({"version":1,"legend":{"#":{"terrain":"NONE","caveInfo":[]}}})");
    TownPreferencesLegend legend;
    const auto parse_cell = +[](const nlohmann::json &, QuestLegendCell &cell, std::string &) -> tl::expected<void, ParseErrorType> {
        cell.object_is_quest_reward = true;
        cell.grid.trap = 1;
        return {};
    };
    ScopedMessageCapture diagnostics;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_cell)) == ParseErrorType::INVALID_VALUE);
    CHECK(legend.empty());
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages().front() == fmt::format(_("町の配置指定が不正です: legend['#'].trap: {}", "Invalid town placement: legend['#'].trap: {}"), describe_legend_object_trap_conflict()));
}
