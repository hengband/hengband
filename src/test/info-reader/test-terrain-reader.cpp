#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/terrain-reader.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <algorithm>
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
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

TEST_CASE("TerrainReader leaves an existing terrain unchanged after a late parse error")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto before = TerrainList::get_instance().get_terrain(0);
    const auto size_before = TerrainList::get_instance().size();

    data["key"] = "FLOOR";
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    data["map_priority"] = 19;
    data["generation"]["changes"] = { { { "terrain", "WALL" }, { "probability", 90 } } };
    data["interactions"] = { { "DESTROYED", 42 } };
    CHECK(TerrainReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);

    const auto &after = TerrainList::get_instance().get_terrain(0);
    CHECK(TerrainList::get_instance().size() == size_before);
    CHECK(after.tag == before.tag);
    CHECK(after.name == before.name);
    CHECK(after.priority == before.priority);
    CHECK(after.flags == before.flags);
    REQUIRE(after.generation_changes.size() == before.generation_changes.size());
    CHECK(after.generation_changes[0].result_tag == before.generation_changes[0].result_tag);
    CHECK(after.generation_changes[0].probability == before.generation_changes[0].probability);
}

TEST_CASE("TerrainReader accepts single-byte symbol characters")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    std::string character = ".";
    SUBCASE("visible character") {}
    SUBCASE("space")
    {
        character = " ";
    }
    data["symbol"]["character"] = character;
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    const auto &terrain = TerrainList::get_instance().get_terrain(0);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(terrain.symbol_definitions.at(lighting).character == character.front());
    }
}

TEST_CASE("TerrainReader rejects invalid symbol lengths without publishing a terrain")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    REQUIRE(TerrainReader(data).read() == PARSE_ERROR_NONE);
    auto &terrains = TerrainList::get_instance();
    const auto before = terrains.get_terrain(0);
    const auto size_before = terrains.size();

    std::string character = "..";
    int expected = PARSE_ERROR_INVALID_VALUE;
    SUBCASE("multiple ASCII characters") {}
    SUBCASE("empty string keeps its existing error")
    {
        character.clear();
        expected = PARSE_ERROR_GENERIC;
    }
    SUBCASE("two-byte UTF-8 character")
    {
        character = "\xc3\xa9";
    }
    SUBCASE("four-byte UTF-8 character")
    {
        character = "\xf0\x9f\x8c\xb2";
    }
    data["symbol"]["character"] = character;
    data["name"]["ja"] = "replacement";
    data["name"]["en"] = "replacement";
    CHECK(TerrainReader(data).read() == expected);
    CHECK(terrains.size() == size_before);
    const auto &after = terrains.get_terrain(0);
    CHECK(after.name == before.name);
    CHECK(after.tag == before.tag);
    for (int lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
        CHECK(after.symbol_definitions.at(lighting) == before.symbol_definitions.at(lighting));
    }

    data["id"] = size_before + 1;
    CHECK(TerrainReader(data).read() == expected);
    CHECK(terrains.size() == size_before);
    CHECK(terrains.get_terrain(0).name == before.name);
}

TEST_CASE("TerrainReader does not grow the terrain list after a failed new entry")
{
    TerrainStateGuard guard;
    auto data = make_terrain();
    const auto size_before = TerrainList::get_instance().size();
    data["id"] = size_before + 1;
    data["interactions"] = { { "DESTROYED", 42 } };
    CHECK(TerrainReader(data).read() == PARSE_ERROR_TOO_FEW_ARGUMENTS);
    CHECK(TerrainList::get_instance().size() == size_before);
}
