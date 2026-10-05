#include "floor/fixed-map-generator.h"
#include "game-option/birth-options.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "io/files-util.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "target/target.h"
#include "term/z-util.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-restore.h"
#include "test/system/dungeon-list-test-access.h"
#include "test/system/terrain-list-test-access.h"
#include "tracking/health-bar-tracker.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace test {
class FixedMapMonraceTestAccess {
public:
    FixedMapMonraceTestAccess()
        : monraces(MonraceList::get_instance())
    {
        monraces.monraces.swap(saved_monraces);
    }

    ~FixedMapMonraceTestAccess()
    {
        monraces.monraces.swap(saved_monraces);
    }

    FixedMapMonraceTestAccess(const FixedMapMonraceTestAccess &) = delete;
    FixedMapMonraceTestAccess &operator=(const FixedMapMonraceTestAccess &) = delete;
    FixedMapMonraceTestAccess(FixedMapMonraceTestAccess &&) = delete;
    FixedMapMonraceTestAccess &operator=(FixedMapMonraceTestAccess &&) = delete;

private:
    MonraceList &monraces;
    MonraceList::Container saved_monraces;
};
}

namespace {
struct TownMapLoadFailure {
};

void throw_town_load_failure(std::string_view)
{
    throw TownMapLoadFailure{};
}

class FixedMapState {
public:
    FixedMapState()
        : terrains(TerrainList::get_instance())
        , saved_terrains(terrains.begin(), terrains.end())
        , saved_flags(init_flags)
        , saved_panel_y(panel_row_min)
        , saved_panel_x(panel_col_min)
    {
        std::copy(std::begin(letter), std::end(letter), saved_letters.begin());
        terrains.resize(3);
        for (auto &terrain : terrains) {
            terrain.flags.clear();
        }
        terrains.get_terrain(0).tag = "NONE";
        terrains.get_terrain(1).tag = "FLOOR";
        terrains.get_terrain(2).tag = "GRANITE";
        DungeonList::get_instance().emplace(DungeonId::WILDERNESS, DungeonDefinition{});
        letter['A'] = {};
        letter['A'].feature = 1;
        letter['A'].cave_info = CAVE_GLOW | CAVE_ROOM;
        letter['A'].special = 7;
        letter['B'] = {};
        letter['B'].feature = 2;
        init_flags = static_cast<init_flags_type>(0);
    }

    ~FixedMapState()
    {
        std::copy(saved_letters.begin(), saved_letters.end(), std::begin(letter));
        terrains.resize(saved_terrains.size());
        std::copy(saved_terrains.begin(), saved_terrains.end(), terrains.begin());
        init_flags = saved_flags;
        panel_row_min = saved_panel_y;
        panel_col_min = saved_panel_x;
    }

    FixedMapState(const FixedMapState &) = delete;
    FixedMapState &operator=(const FixedMapState &) = delete;
    FixedMapState(FixedMapState &&) = delete;
    FixedMapState &operator=(FixedMapState &&) = delete;

private:
    test::DungeonListTestAccess dungeon_state;
    TerrainList &terrains;
    std::vector<TerrainType> saved_terrains;
    std::array<dungeon_grid, 255> saved_letters;
    decltype(init_flags) saved_flags;
    POSITION saved_panel_y;
    POSITION saved_panel_x;
};

class FixedMapFixture {
public:
    FixedMapFixture(int ymin = 3, int xmin = 4, int ymax = 8, int xmax = 7)
        : y(ymin)
        , x(xmin)
    {
        player.current_floor_ptr = &floor;
        initialize_quest_generator_type(&generator, ymin, xmin, ymax, xmax, &y, &x);
    }

    FixedMapFixture(const FixedMapFixture &) = delete;
    FixedMapFixture &operator=(const FixedMapFixture &) = delete;
    FixedMapFixture(FixedMapFixture &&) = delete;
    FixedMapFixture &operator=(FixedMapFixture &&) = delete;

    void legacy(std::string line)
    {
        generator.buf = line.data();
        REQUIRE(generate_fixed_map_floor(&player, &generator) == PARSE_ERROR_NONE);
    }

    FixedMapState state;
    FloorType floor;
    PlayerType player;
    int y;
    int x;
    qtwg_type generator{};
};

class TownFiles {
public:
    TownFiles()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (auto i = 0; i < 100; ++i) {
            auto candidate = std::filesystem::temp_directory_path() / ("hengband-town-pd-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (std::filesystem::create_directory(candidate)) {
                directory = std::move(candidate);
                return;
            }
        }
        throw std::runtime_error("Cannot create isolated town test directory");
    }

    ~TownFiles()
    {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }

    TownFiles(const TownFiles &) = delete;
    TownFiles &operator=(const TownFiles &) = delete;
    TownFiles(TownFiles &&) = delete;
    TownFiles &operator=(TownFiles &&) = delete;

    void write(std::string_view name, const nlohmann::json &data) const
    {
        const auto path = directory / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output << "// Isolated town application fixture\n"
               << data.dump() << '\n';
        output.close();
    }

    std::filesystem::path directory;
};

nlohmann::json make_application_town()
{
    return {
        { "version", 2 },
        { "featureRules", { { { "symbol", "B" }, { "definition", { { "terrain", "GRANITE" }, { "caveInfo", CAVE_ROOM }, { "monster", "0" }, { "object", "0" }, { "ego", "0" }, { "artifact", "0" }, { "trap", "NONE" }, { "special", 17 } } } } } },
        { "buildingRules", nlohmann::json::array() },
        { "mapVariants", { { { "when", "[EQU $LEVEL 7]" }, { "rows", { "AB", "BA" } } }, { { "when", "[EQU $LEVEL 8]" }, { "rows", { "BB", "BB" } } } } },
        { "startingPositions", { { { "when", "[EQU $LEVEL 7]" }, { "y", 1 }, { "x", 1 } }, { { "when", "[EQU $LEVEL 8]" }, { "y", 0 }, { "x", 1 } }, { { "when", "[EQU $LEVEL 7]" }, { "y", 0 }, { "x", 0 } } } },
    };
}

void write_application_town(const TownFiles &files, const nlohmann::json &town)
{
    files.write("TownPreferences.jsonc", { { "version", 1 }, { "legend", { { "A", { { "terrain", "FLOOR" }, { "caveInfo", { "MARK", "GLOW" } }, { "special", 5 } } } } } });
    files.write("TownDefinitionList.jsonc", { { "version", 1 }, { "towns", { { std::to_string(AngbandWorld::get_instance().get_town_index()), "towns/fixture.jsonc" } } } });
    files.write("towns/fixture.jsonc", town);
}
}

TEST_CASE("FixedMapPD row uses origin clips columns and advances empty rows")
{
    FixedMapFixture fixture;
    apply_fixed_map_row(&fixture.player, &fixture.generator, "ABAA");
    CHECK(fixture.y == 4);
    CHECK(fixture.x == 7);
    CHECK(fixture.floor.grid_array[3][3].feat == 0);
    CHECK(fixture.floor.grid_array[3][4].feat == 1);
    CHECK(fixture.floor.grid_array[3][5].feat == 2);
    CHECK(fixture.floor.grid_array[3][6].feat == 1);
    CHECK(fixture.floor.grid_array[3][7].feat == 0);
    CHECK(fixture.floor.grid_array[3][4].info == (CAVE_GLOW | CAVE_ROOM));
    CHECK(fixture.floor.grid_array[3][4].special == 7);
    apply_fixed_map_row(&fixture.player, &fixture.generator, "");
    CHECK(fixture.y == 5);
    CHECK(fixture.x == 4);
    apply_fixed_map_row(&fixture.player, &fixture.generator, "B");
    CHECK(fixture.y == 6);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[5][4].feat == 2);
}

TEST_CASE("FixedMapPD only buildings leaves row and cursors untouched")
{
    FixedMapFixture fixture;
    init_flags = INIT_ONLY_BUILDINGS;
    fixture.x = 6;
    apply_fixed_map_row(&fixture.player, &fixture.generator, "AB");
    CHECK(fixture.y == 3);
    CHECK(fixture.x == 6);
    CHECK(fixture.floor.grid_array[3][4].feat == 0);
    fixture.legacy("D:AB");
    CHECK(fixture.y == 3);
    CHECK(fixture.x == 6);
}

TEST_CASE("FixedMapPD only features preserves non terrain grid state")
{
    FixedMapFixture fixture;
    init_flags = INIT_ONLY_FEATURES;
    auto &grid = fixture.floor.grid_array[3][4];
    grid.info = CAVE_MARK;
    grid.special = 11;
    grid.mimic = 2;
    letter['A'].trap = 2;
    apply_fixed_map_row(&fixture.player, &fixture.generator, "A");
    CHECK(grid.feat == 1);
    CHECK(grid.info == CAVE_MARK);
    CHECK(grid.special == 11);
    CHECK(grid.mimic == 2);
    CHECK(fixture.y == 4);
    CHECK(fixture.x == 5);
}

TEST_CASE("FixedMapPD direct and legacy rows apply identical trap grids")
{
    FixedMapFixture fixture;
    letter['A'].trap = 2;
    apply_fixed_map_row(&fixture.player, &fixture.generator, "A");
    fixture.legacy("D:A");
    const auto &direct = fixture.floor.grid_array[3][4];
    const auto &legacy = fixture.floor.grid_array[4][4];
    CHECK(direct.feat == 2);
    CHECK(direct.mimic == 1);
    CHECK(direct.feat == legacy.feat);
    CHECK(direct.mimic == legacy.mimic);
    CHECK(direct.info == legacy.info);
    CHECK(direct.special == legacy.special);
}

TEST_CASE("FixedMapPD row handles full border empty width and bounded view")
{
    FixedMapFixture fixture(MAX_HGT - 1, MAX_WID - 1, MAX_HGT, MAX_WID);
    const std::string row = "AB";
    apply_fixed_map_row(&fixture.player, &fixture.generator, std::string_view(row.data(), 1));
    CHECK(fixture.floor.grid_array[MAX_HGT - 1][MAX_WID - 1].feat == 1);
    CHECK(fixture.y == MAX_HGT);
    CHECK(fixture.x == MAX_WID);
    fixture.y = 0;
    fixture.generator.xmin = MAX_WID;
    apply_fixed_map_row(&fixture.player, &fixture.generator, "A");
    CHECK(fixture.y == 1);
    CHECK(fixture.x == MAX_WID);
}

TEST_CASE("FixedMapPD row preserves legacy embedded null termination")
{
    FixedMapFixture fixture;
    const std::string row("A\0B", 3);
    apply_fixed_map_row(&fixture.player, &fixture.generator, row);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[3][4].feat == 1);
    CHECK(fixture.floor.grid_array[3][5].feat == 0);
    fixture.legacy("D:" + row);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[4][4].feat == 1);
    CHECK(fixture.floor.grid_array[4][5].feat == 0);
}

TEST_CASE("FixedMapPD legacy malformed start updates dimensions before conversion")
{
    FixedMapFixture fixture;
    init_flags = INIT_CREATE_DUNGEON;
    fixture.y = SCREEN_HGT + 1;
    fixture.x = SCREEN_WID + 1;
    std::string line = "P:invalid:3";
    fixture.generator.buf = line.data();
    CHECK_THROWS_AS(generate_fixed_map_floor(&fixture.player, &fixture.generator), std::invalid_argument);
    CHECK(fixture.floor.height == 2 * SCREEN_HGT);
    CHECK(fixture.floor.width == 2 * SCREEN_WID);
    CHECK(panel_row_min == fixture.floor.height);
    CHECK(panel_col_min == fixture.floor.width);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 0);
    line = "P:2:invalid";
    fixture.generator.buf = line.data();
    CHECK_THROWS_AS(generate_fixed_map_floor(&fixture.player, &fixture.generator), std::invalid_argument);
    CHECK(fixture.player.oldpy == 2);
    CHECK(fixture.player.oldpx == 0);
}

TEST_CASE("FixedMapPD start requires create flag and retains either old coordinate")
{
    FixedMapFixture fixture;
    fixture.floor.height = 9;
    fixture.floor.width = 10;
    apply_fixed_map_start(&fixture.player, &fixture.generator, 1, 2);
    CHECK(fixture.floor.height == 9);
    CHECK(fixture.floor.width == 10);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 0);
    init_flags = INIT_CREATE_DUNGEON;
    fixture.y = SCREEN_HGT + 1;
    fixture.x = SCREEN_WID;
    apply_fixed_map_start(&fixture.player, &fixture.generator, 1, 2);
    CHECK(fixture.floor.height == 2 * SCREEN_HGT);
    CHECK(fixture.floor.width == SCREEN_WID);
    CHECK(panel_row_min == fixture.floor.height);
    CHECK(panel_col_min == fixture.floor.width);
    CHECK(fixture.player.oldpy == 1);
    CHECK(fixture.player.oldpx == 2);
    apply_fixed_map_start(&fixture.player, &fixture.generator, 3, 4);
    CHECK(fixture.player.oldpy == 1);
    CHECK(fixture.player.oldpx == 2);
    fixture.player.oldpx = 0;
    apply_fixed_map_start(&fixture.player, &fixture.generator, 3, 4);
    CHECK(fixture.player.oldpy == 1);
    CHECK(fixture.player.oldpx == 0);
    fixture.player.oldpy = 0;
    fixture.player.oldpx = 2;
    fixture.legacy("P:ignored:ignored");
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 2);
}

TEST_CASE("FixedMapPD legacy and direct starts retain exact panel dimensions")
{
    FixedMapFixture fixture;
    init_flags = INIT_CREATE_DUNGEON;
    fixture.y = SCREEN_HGT;
    fixture.x = SCREEN_WID + 1;
    fixture.legacy("P:2:3");
    CHECK(fixture.player.oldpy == 2);
    CHECK(fixture.player.oldpx == 3);
    CHECK(fixture.floor.height == SCREEN_HGT);
    CHECK(fixture.floor.width == 2 * SCREEN_WID);
    fixture.player.oldpy = fixture.player.oldpx = 0;
    apply_fixed_map_start(&fixture.player, &fixture.generator, 2, 3);
    CHECK(fixture.player.oldpy == 2);
    CHECK(fixture.player.oldpx == 3);
    CHECK(fixture.floor.height == SCREEN_HGT);
    CHECK(fixture.floor.width == 2 * SCREEN_WID);
}

TEST_CASE("FixedMapPD quest start updates current position rather than old position")
{
    FixedMapFixture fixture;
    init_flags = INIT_CREATE_DUNGEON;
    fixture.floor.quest_number = static_cast<QuestId>(1);
    fixture.player.oldpy = 9;
    fixture.player.oldpx = 10;
    fixture.y = 10;
    fixture.x = 10;
    apply_fixed_map_start(&fixture.player, &fixture.generator, 2, 3);
    CHECK(fixture.player.get_position() == Pos2D(2, 3));
    CHECK(fixture.player.oldpy == 9);
    CHECK(fixture.player.oldpx == 10);
    CHECK(fixture.floor.grid_array[2][3].m_idx == 0);
    fixture.legacy("P:3:4");
    CHECK(fixture.player.get_position() == Pos2D(3, 4));
}

TEST_CASE("FixedMapPD town JSONC files select conditional rows and starts")
{
    FixedMapFixture fixture;
    test::ScopedReaderState reader_state;
    test::TerrainListTestAccess floor_tag(TerrainTag::FLOOR, 1);
    test::TerrainListTestAccess wall_tag(TerrainTag::GRANITE_WALL, 2);
    test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    const auto restore = test::scoped_restore(ANGBAND_DIR_EDIT, vanilla_town, lite_town, quit_aux);
    TownFiles files;
    write_application_town(files, make_application_town());
    ANGBAND_DIR_EDIT = files.directory;
    vanilla_town = lite_town = false;
    quit_aux = throw_town_load_failure;
    init_flags = INIT_CREATE_DUNGEON;
    fixture.player.lev = 7;
    REQUIRE_NOTHROW(load_town_map(&fixture.player));
    CHECK(fixture.floor.grid_array[0][0].feat == 1);
    CHECK(fixture.floor.grid_array[0][0].info == (CAVE_MARK | CAVE_GLOW));
    CHECK(fixture.floor.grid_array[0][0].special == 5);
    CHECK(fixture.floor.grid_array[0][1].feat == 2);
    CHECK(fixture.floor.grid_array[0][1].info == CAVE_ROOM);
    CHECK(fixture.floor.grid_array[0][1].special == 17);
    CHECK(fixture.floor.grid_array[1][0].feat == 2);
    CHECK(fixture.floor.grid_array[1][1].feat == 1);
    CHECK(fixture.floor.grid_array[0][2].feat == 0);
    CHECK(fixture.player.oldpy == 1);
    CHECK(fixture.player.oldpx == 1);
    CHECK(fixture.floor.height == SCREEN_HGT);
    CHECK(fixture.floor.width == SCREEN_WID);
    fixture.player.lev = 8;
    fixture.player.oldpy = fixture.player.oldpx = 0;
    REQUIRE_NOTHROW(load_town_map(&fixture.player));
    CHECK(fixture.floor.grid_array[0][0].feat == 2);
    CHECK(fixture.floor.grid_array[1][1].feat == 2);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 1);
}

TEST_CASE("FixedMapPD town JSONC honors only features and only buildings")
{
    FixedMapFixture fixture;
    test::ScopedReaderState reader_state;
    test::TerrainListTestAccess floor_tag(TerrainTag::FLOOR, 1);
    test::TerrainListTestAccess wall_tag(TerrainTag::GRANITE_WALL, 2);
    test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    const auto restore = test::scoped_restore(ANGBAND_DIR_EDIT, vanilla_town, lite_town, quit_aux);
    TownFiles files;
    write_application_town(files, make_application_town());
    ANGBAND_DIR_EDIT = files.directory;
    vanilla_town = lite_town = false;
    quit_aux = throw_town_load_failure;
    fixture.player.lev = 7;
    init_flags = INIT_ONLY_FEATURES;
    auto &grid = fixture.floor.grid_array[0][0];
    grid.info = CAVE_ROOM;
    grid.special = 19;
    REQUIRE_NOTHROW(load_town_map(&fixture.player));
    CHECK(grid.feat == 1);
    CHECK(grid.info == CAVE_ROOM);
    CHECK(grid.special == 19);
    CHECK(fixture.floor.height == 0);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 0);
    init_flags = static_cast<init_flags_type>(INIT_ONLY_BUILDINGS | INIT_CREATE_DUNGEON);
    std::filesystem::remove(files.directory / "TownPreferences.jsonc");
    grid.feat = 2;
    REQUIRE_NOTHROW(load_town_map(&fixture.player));
    CHECK(grid.feat == 2);
    CHECK(grid.info == CAVE_ROOM);
    CHECK(grid.special == 19);
    CHECK(fixture.floor.height == 0);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 0);
}

TEST_CASE("FixedMapPD town JSONC rejects ambiguous maps and missing selected starts")
{
    FixedMapFixture fixture;
    test::ScopedReaderState reader_state;
    test::TerrainListTestAccess floor_tag(TerrainTag::FLOOR, 1);
    test::TerrainListTestAccess wall_tag(TerrainTag::GRANITE_WALL, 2);
    test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    const auto restore = test::scoped_restore(ANGBAND_DIR_EDIT, vanilla_town, lite_town, quit_aux);
    TownFiles files;
    auto town = make_application_town();
    town["mapVariants"][1]["when"] = "[EQU $LEVEL 7]";
    write_application_town(files, town);
    ANGBAND_DIR_EDIT = files.directory;
    vanilla_town = lite_town = false;
    quit_aux = throw_town_load_failure;
    fixture.player.lev = 7;
    init_flags = INIT_CREATE_DUNGEON;
    CHECK_THROWS_AS(load_town_map(&fixture.player), TownMapLoadFailure);
    CHECK(fixture.floor.grid_array[0][0].feat == 0);
    CHECK(fixture.player.oldpy == 0);
    town["mapVariants"][0]["when"] = "[EQU $LEVEL 8]";
    town["mapVariants"][1]["when"] = "[EQU $LEVEL 8]";
    files.write("towns/fixture.jsonc", town);
    CHECK_THROWS_AS(load_town_map(&fixture.player), TownMapLoadFailure);
    CHECK(fixture.floor.grid_array[0][0].feat == 0);
    CHECK(fixture.player.oldpx == 0);
}

TEST_CASE("FixedMapPD quest start deletes an occupying monster and decrements its population")
{
    FixedMapFixture fixture;
    auto &monraces = MonraceList::get_instance();
    const MonraceList::Container previous_monraces(monraces.begin(), monraces.end());
    {
        test::FixedMapMonraceTestAccess monrace_state;
        const auto monrace_id = static_cast<MonraceId>(1);
        auto &monrace = monraces.emplace(monrace_id);
        monrace.cur_num = 1;
        MONSTER_IDX index = 1;
        while (index == Target::get_last_target().get_m_idx() || HealthBarTracker::get_instance().is_tracking(index)) {
            ++index;
        }
        auto &monster = fixture.floor.m_list[index];
        monster.r_idx = monster.ap_r_idx = monrace_id;
        monster.current_floor_ptr = &fixture.floor;
        monster.set_position(Pos2D(2, 3));
        fixture.floor.grid_array[2][3].m_idx = index;
        fixture.floor.m_cnt = 1;
        fixture.floor.m_max = index + 1;
        fixture.floor.quest_number = static_cast<QuestId>(1);
        fixture.y = fixture.x = 10;
        init_flags = INIT_CREATE_DUNGEON;
        REQUIRE(monster.is_valid());
        apply_fixed_map_start(&fixture.player, &fixture.generator, 2, 3);
        CHECK(fixture.player.get_position() == Pos2D(2, 3));
        CHECK(fixture.floor.grid_array[2][3].m_idx == 0);
        CHECK_FALSE(monster.is_valid());
        CHECK(fixture.floor.m_cnt == 0);
        CHECK(monrace.cur_num == 0);
    }
    REQUIRE(monraces.size() == previous_monraces.size());
    for (const auto &[id, previous] : previous_monraces) {
        CHECK(monraces.get_monrace_shared(id) == previous);
    }
}

TEST_CASE("FixedMapPD monrace fixture restores shared definition identity")
{
    test::FixedMapMonraceTestAccess original_state;
    auto &monraces = MonraceList::get_instance();
    const auto original_id = static_cast<MonraceId>(1);
    monraces.emplace(original_id).cur_num = 9;
    const auto original = monraces.get_monrace_shared(original_id);
    {
        test::FixedMapMonraceTestAccess temporary_state;
        CHECK(monraces.empty());
        monraces.emplace(static_cast<MonraceId>(2)).cur_num = 1;
    }
    REQUIRE(monraces.size() == 1);
    CHECK(monraces.get_monrace_shared(original_id) == original);
    CHECK(original->cur_num == 9);
}
