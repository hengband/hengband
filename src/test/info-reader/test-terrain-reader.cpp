#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/terrain-reader.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <vector>

namespace {
class TerrainStateGuard {
public:
    TerrainStateGuard()
        : terrains(TerrainList::get_instance())
        , saved_terrains(terrains.begin(), terrains.end())
        , saved_error_idx(error_idx)
    {
    }

    TerrainStateGuard(const TerrainStateGuard &) = delete;
    TerrainStateGuard &operator=(const TerrainStateGuard &) = delete;

    ~TerrainStateGuard()
    {
        terrains.resize(saved_terrains.size());
        std::copy(saved_terrains.begin(), saved_terrains.end(), terrains.begin());
        error_idx = saved_error_idx;
    }

private:
    TerrainList &terrains;
    std::vector<TerrainType> saved_terrains;
    int saved_error_idx;
};

nlohmann::json make_terrain()
{
    return {
        { "id", 0 },
        { "key", "NONE" },
        { "name", { { "ja", "test terrain" }, { "en", "test terrain" } } },
        { "symbol", { { "character", "." }, { "color", "White" }, { "lit", false } } },
        { "map_priority", 7 },
        { "flags", { "LOS" } },
        { "generation", { { "changes", { { { "terrain", "FLOOR" }, { "probability", 25 } }, { { "terrain", "WALL" }, { "probability", 40 } } } } } },
    };
}
}

TEST_CASE("TerrainReader reads a valid terrain and replaces generation changes on reload")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    CHECK(error_idx == 0);
    CHECK(terrain.idx == 0);
    CHECK(terrain.tag == "NONE");
    CHECK(terrain.name == "test terrain");
    CHECK(terrain.priority == 7);
    CHECK(terrain.has(TerrainCharacteristics::LOS));
    REQUIRE(terrain.generation_changes.size() == 2);
    CHECK(terrain.generation_changes[0].result_tag == "FLOOR");
    CHECK(terrain.generation_changes[0].probability == 25);
    CHECK(terrain.generation_changes[1].result_tag == "WALL");
    CHECK(terrain.generation_changes[1].probability == 40);

    data["generation"]["changes"] = { { { "terrain", "FLOOR" }, { "probability", 100 } } };
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    REQUIRE(terrain.generation_changes.size() == 1);
    CHECK(terrain.generation_changes[0].result_tag == "FLOOR");
    CHECK(terrain.generation_changes[0].probability == 100);
}
