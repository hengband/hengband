#include "info-reader/dungeon-reader.h"
#include "info-reader/parse-error-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/system/dungeon-list-test-access.h"
#include <algorithm>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {
class DungeonReaderStateGuard {
public:
    DungeonReaderStateGuard()
        : terrains(TerrainList::get_instance())
        , previous_terrains(terrains.begin(), terrains.end())
    {
        terrains.resize(2);
        terrains.get_terrain(0).tag = "FLOOR";
        terrains.get_terrain(1).tag = "GRANITE";
    }

    DungeonReaderStateGuard(const DungeonReaderStateGuard &) = delete;
    DungeonReaderStateGuard &operator=(const DungeonReaderStateGuard &) = delete;

    ~DungeonReaderStateGuard()
    {
        terrains.resize(previous_terrains.size());
        std::copy(previous_terrains.begin(), previous_terrains.end(), terrains.begin());
    }

private:
    test::ScopedReaderState reader_state;
    test::DungeonListTestAccess dungeon_state;
    TerrainList &terrains;
    std::vector<TerrainType> previous_terrains;
};

nlohmann::json make_dungeon(int id = 0)
{
    return {
        { "id", id },
        { "name", { { "ja", "test dungeon" }, { "en", "test dungeon" } } },
        { "position", { { "wild_y", 0 }, { "wild_x", 0 } } },
        { "generation", { { "minDepth", 0 }, { "maxDepth", 0 }, { "minPlayerLevel", 0 }, { "objGood", 0 }, { "objGreat", 0 }, { "pit", nlohmann::json::array() }, { "nest", nlohmann::json::array() } } },
        { "floor", { { "tiles", { { { "type", "FLOOR" }, { "rate", 1 } } } }, { "tunnelRate", 50 } } },
        { "wall", { { "tiles", { { { "type", "GRANITE" }, { "rate", 1 } } } }, { "outer", "GRANITE" }, { "inner", "GRANITE" } } },
        { "monsters", { { "minCount", 14 }, { "additionalSpawnProbability", 6250 }, { "normalMonsterRate", 100 }, { "flagsMode", "NONE" } } },
    };
}

const ProbabilityTable<short> &terrain_table(const DungeonDefinition &dungeon, const std::string &section)
{
    return section == "floor" ? dungeon.prob_table_floor : dungeon.prob_table_wall;
}
}

TEST_CASE("DungeonReader reads floor and wall probabilities")
{
    DungeonReaderStateGuard guard;
    const auto data = make_dungeon();
    REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
    const auto &dungeons = DungeonList::get_instance();
    REQUIRE(dungeons.size() == 1);
    const auto &dungeon = dungeons.get_dungeon(DungeonId::WILDERNESS);
    CHECK(dungeon.name == "test dungeon");
    CHECK(dungeon.prob_table_floor.item_count() == 1);
    CHECK(dungeon.prob_table_floor.total_prob() == 1);
    CHECK(dungeon.prob_table_wall.item_count() == 1);
    CHECK(dungeon.prob_table_wall.total_prob() == 1);
}

TEST_CASE("DungeonReader accepts bounded weights and excludes zero weight entries")
{
    for (const std::string section : { "floor", "wall" }) {
        CAPTURE(section);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data[section]["tiles"] = { { { "type", "FLOOR" }, { "rate", 0 } }, { { "type", "GRANITE" }, { "rate", 32767 } } };
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto &table = terrain_table(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS), section);
        CHECK(table.item_count() == 1);
        CHECK(table.total_prob() == 32767);
    }
}

TEST_CASE("DungeonReader rejects invalid weights without publishing a dungeon")
{
    const std::vector<nlohmann::json> invalid_rates = {
        -1,
        32768,
        65537,
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<uint64_t>::max(),
        1.5,
        "1",
        true,
        nullptr,
    };
    for (const std::string section : { "floor", "wall" }) {
        for (const auto &rate : invalid_rates) {
            CAPTURE(section);
            CAPTURE(rate);
            DungeonReaderStateGuard guard;
            auto data = make_dungeon();
            data[section]["tiles"][0]["rate"] = rate;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader rejects unusable or malformed terrain tables")
{
    const std::vector<nlohmann::json> invalid_tiles = {
        nlohmann::json::array(),
        { { { "type", "FLOOR" }, { "rate", 0 } }, { { "type", "GRANITE" }, { "rate", 0 } } },
        { { { "type", "FLOOR" } } },
        { { { "type", "UNKNOWN" }, { "rate", 1 } } },
        { { { "type", "UNKNOWN" }, { "rate", 0 } } },
        { { { "type", 1 }, { "rate", 1 } } },
        { 1 },
        nullptr,
        "tiles",
        nlohmann::json::object(),
    };
    for (const std::string section : { "floor", "wall" }) {
        for (const auto &tiles : invalid_tiles) {
            CAPTURE(section);
            CAPTURE(tiles);
            DungeonReaderStateGuard guard;
            auto data = make_dungeon();
            data[section]["tiles"] = tiles;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader does not change published dungeons after a terrain table error")
{
    for (const std::string section : { "floor", "wall" }) {
        CAPTURE(section);
        DungeonReaderStateGuard guard;
        auto &dungeons = DungeonList::get_instance();
        const auto original = make_dungeon();
        REQUIRE(DungeonReader(original).read() == PARSE_ERROR_NONE);
        const auto before = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
        auto data = make_dungeon(1);
        data["name"]["ja"] = "replacement";
        data["name"]["en"] = "replacement";
        data[section]["tiles"][0]["rate"] = 65537;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
        CHECK(dungeons.size() == 1);
        CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
        CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == before);
        CHECK(before->name == "test dungeon");
        CHECK(before->prob_table_floor.total_prob() == 1);
        CHECK(before->prob_table_wall.total_prob() == 1);
    }
}

TEST_CASE("DungeonReader checks the total terrain weight before integer overflow")
{
    for (const std::string section : { "floor", "wall" }) {
        CAPTURE(section);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        const auto tile = nlohmann::json{ { "type", "FLOOR" }, { "rate", 32767 } };
        auto tiles = nlohmann::json::array();
        for (int i = 0; i < std::numeric_limits<int>::max() / 32767; ++i) {
            tiles.push_back(tile);
        }
        tiles.push_back({ { "type", "FLOOR" }, { "rate", 1 } });
        data[section]["tiles"] = tiles;
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        CHECK(terrain_table(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS), section).total_prob() == std::numeric_limits<int>::max());

        data["id"] = 1;
        data[section]["tiles"].push_back({ { "type", "FLOOR" }, { "rate", 1 } });
        CHECK(DungeonReader(data).read() == PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
        CHECK(DungeonList::get_instance().size() == 1);
        CHECK_FALSE(DungeonList::get_instance().contains(DungeonId::ANGBAND));
    }
}
