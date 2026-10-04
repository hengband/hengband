#include "floor/fixed-map-generator.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/system/dungeon-list-test-access.h"
#include "window/main-window-util.h"
#include <algorithm>
#include <array>
#include <doctest/doctest.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
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
