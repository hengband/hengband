#include "artifact/fixed-art-types.h"
#include "info-reader/dungeon-reader.h"
#include "info-reader/parse-error-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
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

TEST_CASE("DungeonReader preserves item chance cap boundaries")
{
    for (const auto *field : { "objGood", "objGreat" }) {
        for (const auto number : { 0, 100 }) {
            CAPTURE(std::string(field));
            CAPTURE(number);
            DungeonReaderStateGuard guard;
            auto data = make_dungeon();
            data["generation"][field] = number;
            REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
            const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
            CHECK(dungeon.obj_good == (std::string(field) == "objGood" ? number : 0));
            CHECK(dungeon.obj_great == (std::string(field) == "objGreat" ? number : 0));
        }
    }
}

TEST_CASE("DungeonReader rejects item chance caps outside zero to one hundred before publishing")
{
    for (const auto *field : { "objGood", "objGreat" }) {
        for (const auto number : { -1, 101, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(number);
            DungeonReaderStateGuard guard;
            const auto original = make_dungeon();
            REQUIRE(DungeonReader(original).read() == PARSE_ERROR_NONE);
            auto data = make_dungeon(1);
            data["name"] = { { "ja", "changed" }, { "en", "changed" } };
            data["generation"][field] = number;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
            REQUIRE(DungeonList::get_instance().size() == 1);
            const auto &previous = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
            CHECK(previous.name == "test dungeon");
            CHECK(previous.obj_good == 0);
            CHECK(previous.obj_great == 0);
        }
    }
}

TEST_CASE("DungeonReader preserves monster count boundaries")
{
    for (const auto number : { 0, 100 }) {
        CAPTURE(number);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["monsters"]["minCount"] = number;
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_monster_count_on_floor == number);
    }
}

TEST_CASE("DungeonReader rejects monster counts outside zero to one hundred before publishing")
{
    for (const auto number : { -1, 101, std::numeric_limits<int>::min(), std::numeric_limits<int>::max() }) {
        CAPTURE(number);
        DungeonReaderStateGuard guard;
        const auto original = make_dungeon();
        REQUIRE(DungeonReader(original).read() == PARSE_ERROR_NONE);
        auto data = make_dungeon(1);
        data["monsters"]["minCount"] = number;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        REQUIRE(DungeonList::get_instance().size() == 1);
        CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_monster_count_on_floor == 14);
    }
}

TEST_CASE("DungeonReader preserves signed short depth bounds and deep dungeons")
{
    for (const auto depth : { 0, 696, 32767 }) {
        CAPTURE(depth);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["generation"]["minDepth"] = depth;
        data["generation"]["maxDepth"] = depth;
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.mindepth == depth);
        CHECK(dungeon.maxdepth == depth);
    }
}

TEST_CASE("DungeonReader rejects depths outside save range and inverted depths")
{
    for (const auto *field : { "minDepth", "maxDepth" }) {
        for (const auto depth : { -1, 32768, std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(depth);
            DungeonReaderStateGuard guard;
            auto data = make_dungeon();
            data["generation"][field] = depth;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
    DungeonReaderStateGuard guard;
    auto data = make_dungeon();
    data["generation"]["minDepth"] = 100;
    data["generation"]["maxDepth"] = 99;
    CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_VALUE);
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader restricts required player level to playable levels")
{
    for (const auto level : { -1, 0, 50, 51, 32767 }) {
        CAPTURE(level);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["generation"]["minPlayerLevel"] = level;
        if (level >= 0 && level <= 50) {
            REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
            CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).min_plev == level);
        } else {
            CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader rejects coordinates outside wilderness storage and IDs outside the dungeon enum")
{
    for (const auto *field : { "wild_y", "wild_x" }) {
        for (const auto coordinate : { -1, 32767, std::numeric_limits<int>::max() }) {
            CAPTURE(std::string(field));
            CAPTURE(coordinate);
            DungeonReaderStateGuard guard;
            auto data = make_dungeon();
            data["position"][field] = coordinate;
            CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
    for (const auto id : { -1, 21, 9999 }) {
        CAPTURE(id);
        DungeonReaderStateGuard guard;
        const auto data = make_dungeon(id);
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader preserves nonpercentage tunnel thresholds within the depth range")
{
    for (const auto threshold : { -1, 0, 1000, 32767, 32768 }) {
        CAPTURE(threshold);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon();
        data["floor"]["tunnelRate"] = threshold;
        if (threshold >= 0 && threshold <= 32767) {
            REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
            CHECK(DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS).tunnel_percent == threshold);
        } else {
            CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
            CHECK(DungeonList::get_instance().empty());
        }
    }
}

TEST_CASE("DungeonReader validates entrances against actual wilderness dimensions without changing definitions")
{
    // 3×4の荒野で端を除いた内側は y=1・x=1〜2 だけ。
    for (const auto &[y, x] : { std::pair{ 1, 1 }, std::pair{ 1, 2 }, std::pair{ 0, 1 }, std::pair{ 2, 1 }, std::pair{ 1, 0 }, std::pair{ 1, 3 }, std::pair{ 3, 0 }, std::pair{ 0, 4 } }) {
        CAPTURE(y);
        CAPTURE(x);
        DungeonReaderStateGuard guard;
        auto data = make_dungeon(1);
        data["position"] = { { "wild_y", y }, { "wild_x", x } };
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto previous = DungeonList::get_instance().get_dungeon_shared(DungeonId::ANGBAND);
        const auto inside = (y == 1) && (x == 1 || x == 2);
        CHECK(DungeonReader::validate_wilderness_positions(3, 4) == (inside ? PARSE_ERROR_NONE : PARSE_ERROR_OUT_OF_BOUNDS));
        CHECK(DungeonList::get_instance().get_dungeon_shared(DungeonId::ANGBAND) == previous);
        CHECK(previous->get_position() == Pos2D(y, x));
    }
    DungeonReaderStateGuard guard;
    auto data = make_dungeon();
    data["position"] = { { "wild_y", 32766 }, { "wild_x", 32766 } };
    REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
    CHECK(DungeonReader::validate_wilderness_positions(3, 4) == PARSE_ERROR_NONE);
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
        10000,
        32767,
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
    for (const auto &[guardian, artifact] : { std::pair{ 1, 0 }, std::pair{ 1, 1 }, std::pair{ 1, 9999 } }) {
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

TEST_CASE("DungeonReader accepts final object IDs through the Baseitem reader limit")
{
    DungeonReaderStateGuard guard;
    auto &items = BaseitemList::get_instance();
    const test::ScopedVectorWrapper saved_items(items);
    items.resize(32768);
    for (const auto id : { 1, 9999, 10000, 32767 }) {
        CAPTURE(id);
        auto data = make_dungeon(id == 1 ? 0 : id == 9999 ? 1
                                           : id == 10000  ? 2
                                                          : 3);
        data["final_floor"] = { { "object", id } };
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto dungeon_id = static_cast<DungeonId>(data["id"].get<int>());
        CHECK(DungeonList::get_instance().get_dungeon(dungeon_id).final_object == id);
    }
}

TEST_CASE("DungeonReader rejects final objects outside the Baseitem limit and missing IDs")
{
    for (const auto id : { -1, 0, 1, 32768, std::numeric_limits<int>::max() }) {
        CAPTURE(id);
        DungeonReaderStateGuard guard;
        auto &items = BaseitemList::get_instance();
        const test::ScopedVectorWrapper saved_items(items);
        auto data = make_dungeon();
        data["final_floor"] = { { "object", id } };
        CHECK(DungeonReader(data).read() == (id == 1 ? PARSE_ERROR_INVALID_VALUE : PARSE_ERROR_INVALID_FLAG));
        CHECK(DungeonList::get_instance().empty());
    }
}

TEST_CASE("DungeonReader rejects guardian zero as below the schema minimum before publication")
{
    const FinalFloorStateGuard state;
    auto data = make_dungeon();
    data["final_floor"] = { { "guardian", 0 } };
    CHECK(DungeonReader(data).read() == PARSE_ERROR_OUT_OF_BOUNDS);
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader propagates nested parse errors without publishing partial definitions")
{
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        parse_error_type error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "/description", 1, PARSE_ERROR_INVALID_TYPE },
        { "/generation", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/minDepth", "depth", PARSE_ERROR_INVALID_TYPE },
        { "/generation/pit", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/pit", { "ORC", "UNKNOWN" }, PARSE_ERROR_INVALID_FLAG },
        { "/generation/nest", { "UNDEAD", 1 }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/generation/nest", { "UNKNOWN" }, PARSE_ERROR_INVALID_FLAG },
        { "/floor", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/floor/tunnelRate", "rate", PARSE_ERROR_INVALID_TYPE },
        { "/wall", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/wall/inner", "UNKNOWN", PARSE_ERROR_UNDEFINED_TERRAIN_TAG },
        { "/wall/streams", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/wall/streams/1/type", "UNKNOWN", PARSE_ERROR_UNDEFINED_TERRAIN_TAG },
        { "/wall/streams/1/count", 0, PARSE_ERROR_INVALID_FLAG },
        { "/wall/streams/1/chance", "chance", PARSE_ERROR_INVALID_TYPE },
        { "/wall/streams/1/priority", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/final_floor", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/final_floor/guardian", 65537, PARSE_ERROR_OUT_OF_BOUNDS },
        { "/final_floor/guardian", 2, PARSE_ERROR_INVALID_VALUE },
        { "/final_floor/object", 0, PARSE_ERROR_INVALID_FLAG },
        { "/final_floor/artifact", 2, PARSE_ERROR_INVALID_VALUE },
        { "/flags", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/flags", { "MAZE", "UNKNOWN" }, PARSE_ERROR_INVALID_FLAG },
        { "/monsters", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/monsters/additionalSpawnProbability", 0, PARSE_ERROR_INVALID_FLAG },
        { "/monsters/flagsMode", 1, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/monsters/flagsMode", "UNKNOWN", PARSE_ERROR_INVALID_FLAG },
        { "/monsters/flags", { "UNIQUE", "UNKNOWN" }, PARSE_ERROR_INVALID_FLAG },
        { "/monsters/symbols", { "D", "XX" }, PARSE_ERROR_INVALID_FLAG },
        { "/monsters/symbols", { "D", 1 }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/monsters/spells", { "HASTE", "UNKNOWN" }, PARSE_ERROR_INVALID_FLAG },
        { "/monsters/spells", { "HASTE", 1 }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
    };
    for (const auto &field : invalid_fields) {
        CAPTURE(field.path);
        CAPTURE(field.value);
        FinalFloorStateGuard guard;
        const auto original = make_dungeon();
        REQUIRE(DungeonReader(original).read() == PARSE_ERROR_NONE);
        const auto &dungeons = DungeonList::get_instance();
        const auto before = dungeons.get_dungeon_shared(DungeonId::WILDERNESS);
        auto data = make_dungeon(1);
        data["wall"]["streams"] = {
            { { "type", "FLOOR" }, { "count", 1 }, { "chance", 1 }, { "priority", 0 } },
            { { "type", "GRANITE" }, { "count", 2 }, { "chance", 2 }, { "priority", 1 } },
        };
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 0 } };
        data[nlohmann::json::json_pointer(field.path)] = field.value;

        CHECK(DungeonReader(data).read() == field.error);
        CHECK(error_idx == 1);
        CHECK(dungeons.size() == 1);
        CHECK_FALSE(dungeons.contains(DungeonId::ANGBAND));
        CHECK(dungeons.get_dungeon_shared(DungeonId::WILDERNESS) == before);
        CHECK(before->name == "test dungeon");
        CHECK(before->streams.empty());
        CHECK(before->prob_table_floor.total_prob() == 1);
        CHECK(before->prob_table_wall.total_prob() == 1);
    }
}

TEST_CASE("DungeonReader preserves error priority and the diagnostic record ID")
{
    DungeonReaderStateGuard guard;
    auto data = make_dungeon(1);
    data["generation"]["pit"] = { "UNKNOWN" };
    data["wall"]["inner"] = "UNKNOWN";
    data["monsters"]["symbols"] = { "XX" };
    SUBCASE("an invalid ID does not update the diagnostic ID")
    {
        data["id"] = "id";
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_TYPE);
        CHECK(error_idx == -1);
    }
    SUBCASE("a nonsequential ID precedes field parsing")
    {
        error_idx = 1;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
        CHECK(error_idx == 1);
    }
    SUBCASE("the valid ID is selected before name parsing")
    {
        data["name"] = 1;
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_TYPE);
        CHECK(error_idx == 1);
    }
    SUBCASE("generation errors precede later field errors")
    {
        CHECK(DungeonReader(data).read() == PARSE_ERROR_INVALID_FLAG);
        CHECK(error_idx == 1);
    }
    CHECK(DungeonList::get_instance().empty());
}

TEST_CASE("DungeonReader publishes complete nested definitions and accepts optional nulls")
{
    FinalFloorStateGuard guard;
    auto &baseitems = BaseitemList::get_instance();
    const auto restore_baseitems = test::ScopedVectorWrapper(baseitems);
    baseitems.resize(2);
    auto data = make_dungeon();
    SUBCASE("populated helpers preserve values and sort streams")
    {
        data["description"] = { { "ja", "description" }, { "en", "description" } };
        data["generation"]["pit"] = { "", "ORC" };
        data["generation"]["nest"] = { "", "UNDEAD" };
        data["wall"]["streams"] = {
            { { "type", "GRANITE" }, { "count", 255 }, { "chance", 65535 }, { "priority", 255 } },
            { { "type", "FLOOR" }, { "count", 1 }, { "chance", 1 }, { "priority", 0 } },
        };
        data["final_floor"] = { { "guardian", 1 }, { "artifact", 1 }, { "object", 1 } };
        data["flags"] = { "", "MAZE" };
        data["monsters"]["flagsMode"] = "AND";
        data["monsters"]["flags"] = { "", "UNIQUE", "IM_ACID" };
        data["monsters"]["symbols"] = { "", "D" };
        data["monsters"]["spells"] = { "", "HASTE" };
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.text == "description");
        CHECK(dungeon.pit.has(PitKind::ORC));
        CHECK(dungeon.nest.has(NestKind::UNDEAD));
        REQUIRE(dungeon.streams.size() == 2);
        CHECK(dungeon.streams[0].terrain_id == 0);
        CHECK(dungeon.streams[0].count == 1);
        CHECK(dungeon.streams[0].chance == 1);
        CHECK(dungeon.streams[0].priority == 0);
        CHECK(dungeon.streams[1].terrain_id == 1);
        CHECK(dungeon.streams[1].count == 255);
        CHECK(dungeon.streams[1].chance == 65535);
        CHECK(dungeon.streams[1].priority == 255);
        CHECK(dungeon.final_guardian == MonraceId::FILTHY_URCHIN);
        CHECK(dungeon.final_artifact == FixedArtifactId::GALADRIEL_PHIAL);
        CHECK(dungeon.final_object == 1);
        CHECK(dungeon.flags.has(DungeonFeatureType::MAZE));
        CHECK(dungeon.mode == DungeonMode::AND);
        CHECK(dungeon.mon_kind_flags.has(MonsterKindType::UNIQUE));
        CHECK(dungeon.mon_resistance_flags.has(MonsterResistanceType::IMMUNE_ACID));
        CHECK(dungeon.mon_ability_flags.has(MonsterAbilityType::HASTE));
        CHECK(dungeon.r_chars == std::vector<char>{ 'D' });
    }
    SUBCASE("optional null fields keep defaults")
    {
        data["description"] = nullptr;
        data["wall"]["streams"] = nullptr;
        data["final_floor"] = nullptr;
        data["flags"] = nullptr;
        for (const auto *key : { "flags", "symbols", "spells" }) {
            data["monsters"][key] = nullptr;
        }
        REQUIRE(DungeonReader(data).read() == PARSE_ERROR_NONE);
        const auto &dungeon = DungeonList::get_instance().get_dungeon(DungeonId::WILDERNESS);
        CHECK(dungeon.text.empty());
        CHECK(dungeon.streams.empty());
        CHECK(dungeon.flags.none());
        CHECK(dungeon.r_chars.empty());
        CHECK(dungeon.mon_kind_flags.none());
        CHECK(dungeon.mon_ability_flags.none());
        CHECK(dungeon.final_guardian == MonraceId{});
        CHECK(dungeon.final_artifact == FixedArtifactId::NONE);
    }
    CHECK(error_idx == 0);
    CHECK(DungeonList::get_instance().size() == 1);
}
