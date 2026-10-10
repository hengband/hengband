#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/parse-error-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/system/artifact-list-test-access.h"
#include "test/system/dungeon-list-test-access.h"
#include "test/system/monrace-list-test-access.h"
#include <algorithm>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
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

struct FinalFloorStateGuard {
    // Monraceのfixtureはerror_idxも変更するため、Dungeonのfixtureより先に置く。
    test::MonraceListTestAccess races;
    test::ArtifactListTestAccess artifacts;
    DungeonReaderStateGuard dungeon;

    FinalFloorStateGuard()
    {
        MonraceList::get_instance().emplace(MonraceId::FILTHY_URCHIN);
        ArtifactList::get_instance().emplace(FixedArtifactId::GALADRIEL_PHIAL, {});
    }

    FinalFloorStateGuard(const FinalFloorStateGuard &) = delete;
    FinalFloorStateGuard(FinalFloorStateGuard &&) = delete;
    FinalFloorStateGuard &operator=(const FinalFloorStateGuard &) = delete;
    FinalFloorStateGuard &operator=(FinalFloorStateGuard &&) = delete;
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

TEST_CASE("DungeonReader rejects player levels outside their storage type")
{
    for (const auto value : { -32769, 65537 }) {
        CAPTURE(value);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["generation"]["minPlayerLevel"] = value;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(DungeonList::get_instance().empty());
    }
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

TEST_CASE("DungeonReader rejects final-floor ID narrowing before reference lookup")
{
    const std::vector<nlohmann::json> invalid_ids{
        -1,
        -32768,
        32768,
        -32769,
        65536,
        -65536,
        65537,
        -65535,
        int64_t{ 4294967297 },
        int64_t{ -4294967295 },
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<uint64_t>::max(),
    };
    for (const auto *field : { "guardian", "artifact" }) {
        for (const auto &value : invalid_ids) {
            CAPTURE(std::string(field));
            CAPTURE(value);
            const FinalFloorStateGuard state;
            const auto original_data = make_dungeon();
            REQUIRE(DungeonReader(original_data).read() == PARSE_ERROR_NONE);
            auto &dungeons = DungeonList::get_instance();
            const auto original = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
            auto data = make_dungeon(1);
            data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 } };
            data["final_floor"][field] = value;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_OUT_OF_BOUNDS);
            CHECK(dungeons.size() == 1);
            CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
            CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == original);
            CHECK(original->final_guardian == MonraceId::PLAYER);
            CHECK(original->final_artifact == FixedArtifactId::NONE);
        }
    }
}

TEST_CASE("DungeonReader preserves representable final-floor IDs and the no-artifact sentinel")
{
    for (const auto &[guardian, artifact] : { std::pair{ 1, 0 }, std::pair{ 1, 1 }, std::pair{ 1, 32767 } }) {
        CAPTURE(guardian);
        CAPTURE(artifact);
        const FinalFloorStateGuard state;
        const auto guardian_id = static_cast<MonraceId>(guardian);
        const auto artifact_id = static_cast<FixedArtifactId>(artifact);
        if (artifact_id != FixedArtifactId::NONE && artifact_id != FixedArtifactId::GALADRIEL_PHIAL) {
            ArtifactList::get_instance().emplace(artifact_id, {});
        }
        auto data = make_dungeon();
        data["final_floor"] = { { "guardian", guardian }, { "artifact", artifact } };
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.final_guardian == guardian_id);
        CHECK(dungeon.final_artifact == artifact_id);
    }
}

TEST_CASE("DungeonReader still rejects undefined representable final-floor IDs")
{
    for (const auto *field : { "guardian", "artifact" }) {
        CAPTURE(std::string(field));
        const FinalFloorStateGuard state;
        auto data = make_dungeon();
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 } };
        data["final_floor"][field] = 2;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_VALUE);
        CHECK(DungeonList::get_instance().empty());
    }
}
