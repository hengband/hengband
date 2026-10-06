#include "dungeon/quest.h"
#include "floor/fixed-map-generator.h"
#include "game-option/birth-options.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "io/files-util.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
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
#include "test/temporary-json-files.h"
#include "tracking/health-bar-tracker.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <doctest/doctest.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
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

class TownFiles : public test::TemporaryJsonFiles {
public:
    TownFiles() = default;
    TownFiles(const TownFiles &) = delete;
    TownFiles &operator=(const TownFiles &) = delete;
    TownFiles(TownFiles &&) = delete;
    TownFiles &operator=(TownFiles &&) = delete;

    void write(std::string_view name, const nlohmann::json &data) const
    {
        this->write_raw(name, "// Isolated town application fixture\n" + data.dump() + '\n');
    }
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

[[nodiscard]] auto scoped_quest_layout_state()
{
    auto &maps = QuestFixedMapList::get_instance();
    auto &system = AngbandSystem::get_instance();
    auto legend = maps.get_base_legend();
    const auto seed = system.get_seed_town();
    const auto previous_leaving = leaving_quest;
    maps.set_base_legend({});
    system.set_seed_town(0);
    leaving_quest = QuestId::NONE;
    return util::make_finalizer([legend = std::move(legend), seed, previous_leaving] {
        QuestFixedMapList::get_instance().set_base_legend(legend);
        AngbandSystem::get_instance().set_seed_town(seed);
        leaving_quest = previous_leaving;
    });
}
}

TEST_CASE("QuestFixedMap JSON rows preserve origin empty rows null termination and final row width")
{
    FixedMapFixture fixture;
    const auto restore = scoped_quest_layout_state();
    QuestType quest;
    QuestFixedMap map;
    map.maps = { { "AB", "", std::string("A\0B", 3), "B" } };
    map.starts = { { tl::nullopt, 2, 0 } };
    fixture.floor.quest_number = QuestId::THIEF;
    fixture.player.oldpy = 9;
    fixture.player.oldpx = 10;
    init_flags = INIT_CREATE_DUNGEON;
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.floor.grid_array[0][0].feat == 1);
    CHECK(fixture.floor.grid_array[0][1].feat == 2);
    CHECK(fixture.floor.grid_array[1][0].feat == 0);
    CHECK(fixture.floor.grid_array[2][0].feat == 1);
    CHECK(fixture.floor.grid_array[2][1].feat == 0);
    CHECK(fixture.floor.grid_array[3][0].feat == 2);
    CHECK(fixture.floor.grid_array[3][1].feat == 0);
    CHECK(fixture.floor.grid_array[0][0].info == (CAVE_GLOW | CAVE_ROOM));
    CHECK(fixture.floor.grid_array[0][0].special == 7);
    CHECK(fixture.player.get_position() == Pos2D(2, 0));
    CHECK(fixture.player.oldpy == 9);
    CHECK(fixture.player.oldpx == 10);
    CHECK(fixture.floor.height == SCREEN_HGT);
    CHECK(fixture.floor.width == SCREEN_WID);
    CHECK(panel_row_min == SCREEN_HGT);
    CHECK(panel_col_min == SCREEN_WID);
    CHECK(map.maps[0][2] == std::string("A\0B", 3));

    map.maps = { { std::string(SCREEN_WID + 1, 'A'), "" } };
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.floor.height == SCREEN_HGT);
    CHECK(fixture.floor.width == 0);
}

TEST_CASE("QuestFixedMap JSON retains quest specific initialization flag behavior")
{
    FixedMapFixture fixture;
    const auto restore = scoped_quest_layout_state();
    QuestType quest;
    QuestFixedMap map;
    map.maps = { { "A" } };
    map.starts = { { tl::nullopt, 2, 3 } };
    fixture.floor.height = 9;
    fixture.floor.width = 10;
    auto &grid = fixture.floor.grid_array[0][0];
    SUBCASE("without create flag rows apply but dimensions and start do not")
    {
        REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
        CHECK(grid.feat == 1);
        CHECK(grid.info == (CAVE_GLOW | CAVE_ROOM));
        CHECK(grid.special == 7);
        CHECK(fixture.floor.height == 9);
        CHECK(fixture.floor.width == 10);
        CHECK(fixture.player.oldpy == 0);
        CHECK(fixture.player.oldpx == 0);
    }
    SUBCASE("only buildings does not skip quest rows unlike town rows")
    {
        init_flags = static_cast<init_flags_type>(INIT_ONLY_BUILDINGS | INIT_CREATE_DUNGEON);
        REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
        CHECK(grid.feat == 1);
        CHECK(grid.info == (CAVE_GLOW | CAVE_ROOM));
        CHECK(grid.special == 7);
        CHECK(fixture.floor.height == SCREEN_HGT);
        CHECK(fixture.floor.width == SCREEN_WID);
        CHECK(fixture.player.oldpy == 2);
        CHECK(fixture.player.oldpx == 3);
    }
    SUBCASE("only features retains non terrain state while applying start")
    {
        init_flags = static_cast<init_flags_type>(INIT_ONLY_FEATURES | INIT_CREATE_DUNGEON);
        grid.info = CAVE_MARK;
        grid.special = 11;
        grid.mimic = 2;
        letter['A'].trap = 2;
        REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
        CHECK(grid.feat == 1);
        CHECK(grid.info == CAVE_MARK);
        CHECK(grid.special == 11);
        CHECK(grid.mimic == 2);
        CHECK(fixture.player.oldpy == 2);
        CHECK(fixture.player.oldpx == 3);
    }
}

TEST_CASE("QuestFixedMap JSON variants use town seed modulo count without changing seed")
{
    FixedMapFixture fixture;
    const auto restore = scoped_quest_layout_state();
    QuestType quest;
    QuestFixedMap map;
    map.maps = { { "A" }, { "B" }, { "AB" } };
    auto &system = AngbandSystem::get_instance();
    for (const auto seed : { 0U, 4U, 5U, 6U }) {
        system.set_seed_town(seed);
        fixture.floor.grid_array[0][1].feat = 0;
        REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
        CHECK(fixture.floor.grid_array[0][0].feat == (seed % 3 == 1 ? 2 : 1));
        CHECK(fixture.floor.grid_array[0][1].feat == (seed % 3 == 2 ? 2 : 0));
        CHECK(system.get_seed_town() == seed);
    }
}

TEST_CASE("QuestFixedMap JSON start selection keeps first match last fallback and absent match behavior")
{
    FixedMapFixture fixture;
    const auto restore = scoped_quest_layout_state();
    QuestType quest;
    QuestFixedMap map;
    map.maps = { { "AB" } };
    map.starts = { { tl::nullopt, 1, 2 }, { 7, 3, 4 }, { tl::nullopt, 5, 6 }, { 7, 8, 9 } };
    fixture.floor.quest_number = QuestId::THIEF;
    init_flags = INIT_CREATE_DUNGEON;
    leaving_quest = static_cast<QuestId>(7);
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.player.get_position() == Pos2D(3, 4));
    leaving_quest = static_cast<QuestId>(8);
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.player.get_position() == Pos2D(5, 6));
    map.starts = { { 7, 3, 4 } };
    fixture.floor.height = 9;
    fixture.floor.width = 10;
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.floor.grid_array[0][0].feat == 1);
    CHECK(fixture.player.get_position() == Pos2D(5, 6));
    CHECK(fixture.floor.height == 9);
    CHECK(fixture.floor.width == 10);
    map.starts.clear();
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(fixture.player.get_position() == Pos2D(5, 6));
    CHECK(fixture.floor.height == 9);
}

TEST_CASE("QuestFixedMap JSON no map applies legend overrides but not selected start")
{
    FixedMapFixture fixture;
    const auto restore = scoped_quest_layout_state();
    QuestType quest;
    QuestLegendCell base;
    base.grid.feature = 2;
    base.grid.special = 13;
    QuestFixedMapList::get_instance().set_base_legend({ { 'A', base }, { 'C', base } });
    QuestFixedMap map;
    map.legend['A'].grid.feature = 1;
    map.legend['A'].grid.special = 17;
    map.starts = { { tl::nullopt, 2, 3 } };
    init_flags = INIT_CREATE_DUNGEON;
    fixture.floor.height = 9;
    fixture.floor.width = 10;
    REQUIRE(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_NONE);
    CHECK(letter['A'].feature == 1);
    CHECK(letter['A'].special == 17);
    CHECK(letter['C'].feature == 2);
    CHECK(letter['C'].special == 13);
    CHECK(fixture.floor.grid_array[0][0].feat == 0);
    CHECK(fixture.floor.height == 9);
    CHECK(fixture.floor.width == 10);
    CHECK(fixture.player.oldpy == 0);
    CHECK(fixture.player.oldpx == 0);
}

TEST_CASE("QuestFixedMap JSON fixture restores base legend seed leaving quest letters flags and panels")
{
    FixedMapFixture outer;
    const auto outer_restore = scoped_quest_layout_state();
    QuestLegendCell original;
    original.grid.feature = 2;
    original.grid.special = 19;
    QuestFixedMapList::get_instance().set_base_legend({ { 'C', original } });
    AngbandSystem::get_instance().set_seed_town(47);
    leaving_quest = QuestId::SEWER;
    init_flags = INIT_ONLY_FEATURES;
    panel_row_min = 17;
    panel_col_min = 18;
    {
        FixedMapFixture inner;
        const auto inner_restore = scoped_quest_layout_state();
        QuestType quest;
        QuestFixedMap map;
        map.legend['A'].grid.feature = 2;
        map.maps = { { "A" } };
        map.starts = { { tl::nullopt, 1, 2 } };
        init_flags = INIT_CREATE_DUNGEON;
        REQUIRE(generate_quest_floor_from_json(&inner.player, quest, map) == PARSE_ERROR_NONE);
        CHECK(letter['A'].feature == 2);
    }
    const auto &legend = QuestFixedMapList::get_instance().get_base_legend();
    REQUIRE(legend.size() == 1);
    CHECK(legend.at('C').grid.feature == 2);
    CHECK(legend.at('C').grid.special == 19);
    CHECK(AngbandSystem::get_instance().get_seed_town() == 47);
    CHECK(leaving_quest == QuestId::SEWER);
    CHECK(letter['A'].feature == 1);
    CHECK(init_flags == INIT_ONLY_FEATURES);
    CHECK(panel_row_min == 17);
    CHECK(panel_col_min == 18);
}

TEST_CASE("FixedMapPD row uses origin clips columns and advances empty rows")
{
    FixedMapFixture fixture;
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "ABAA") == PARSE_ERROR_NONE);
    CHECK(fixture.y == 4);
    CHECK(fixture.x == 7);
    CHECK(fixture.floor.grid_array[3][3].feat == 0);
    CHECK(fixture.floor.grid_array[3][4].feat == 1);
    CHECK(fixture.floor.grid_array[3][5].feat == 2);
    CHECK(fixture.floor.grid_array[3][6].feat == 1);
    CHECK(fixture.floor.grid_array[3][7].feat == 0);
    CHECK(fixture.floor.grid_array[3][4].info == (CAVE_GLOW | CAVE_ROOM));
    CHECK(fixture.floor.grid_array[3][4].special == 7);
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "") == PARSE_ERROR_NONE);
    CHECK(fixture.y == 5);
    CHECK(fixture.x == 4);
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "B") == PARSE_ERROR_NONE);
    CHECK(fixture.y == 6);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[5][4].feat == 2);
}

TEST_CASE("FixedMapPD only buildings leaves row and cursors untouched")
{
    FixedMapFixture fixture;
    init_flags = INIT_ONLY_BUILDINGS;
    fixture.x = 6;
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "AB") == PARSE_ERROR_NONE);
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
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "A") == PARSE_ERROR_NONE);
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
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "A") == PARSE_ERROR_NONE);
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
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, std::string_view(row.data(), 1)) == PARSE_ERROR_NONE);
    CHECK(fixture.floor.grid_array[MAX_HGT - 1][MAX_WID - 1].feat == 1);
    CHECK(fixture.y == MAX_HGT);
    CHECK(fixture.x == MAX_WID);
    fixture.y = 0;
    fixture.generator.xmin = MAX_WID;
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, "A") == PARSE_ERROR_NONE);
    CHECK(fixture.y == 1);
    CHECK(fixture.x == MAX_WID);
}

TEST_CASE("FixedMapPD row preserves legacy embedded null termination")
{
    FixedMapFixture fixture;
    const std::string row("A\0B", 3);
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, row) == PARSE_ERROR_NONE);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[3][4].feat == 1);
    CHECK(fixture.floor.grid_array[3][5].feat == 0);
    fixture.legacy("D:" + row);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[4][4].feat == 1);
    CHECK(fixture.floor.grid_array[4][5].feat == 0);
}

TEST_CASE("FixedMapPD rejects unsafe row bytes before modifying grids or cursors")
{
    for (const auto byte : { 0x1f, 0x7f, 0x80, 0xfe, 0xff }) {
        CAPTURE(byte);
        FixedMapFixture fixture;
        fixture.x = 6;
        auto &grid = fixture.floor.grid_array[3][4];
        grid.feat = 2;
        grid.info = CAVE_MARK;
        grid.special = 19;
        const auto row = "A" + std::string(1, static_cast<char>(byte));
        CHECK(apply_fixed_map_row(&fixture.player, &fixture.generator, row) == PARSE_ERROR_INVALID_VALUE);
        auto line = "D:" + row;
        fixture.generator.buf = line.data();
        CHECK(generate_fixed_map_floor(&fixture.player, &fixture.generator) == PARSE_ERROR_INVALID_VALUE);
        CHECK(fixture.y == 3);
        CHECK(fixture.x == 6);
        CHECK(grid.feat == 2);
        CHECK(grid.info == CAVE_MARK);
        CHECK(grid.special == 19);
        init_flags = INIT_ONLY_BUILDINGS;
        CHECK(apply_fixed_map_row(&fixture.player, &fixture.generator, row) == PARSE_ERROR_NONE);
        CHECK(fixture.y == 3);
        CHECK(fixture.x == 6);
    }
}

TEST_CASE("FixedMapPD ignores bytes after null and accepts printable ASCII boundaries")
{
    FixedMapFixture fixture;
    const std::string row("A\0\xff", 3);
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, row) == PARSE_ERROR_NONE);
    CHECK(fixture.y == 4);
    CHECK(fixture.x == 5);
    CHECK(fixture.floor.grid_array[3][4].feat == 1);
    CHECK(fixture.floor.grid_array[3][5].feat == 0);
    letter[' '] = letter['A'];
    letter['~'] = letter['B'];
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, " ~") == PARSE_ERROR_NONE);
    CHECK(fixture.floor.grid_array[4][4].feat == 1);
    CHECK(fixture.floor.grid_array[4][5].feat == 2);
    REQUIRE(apply_fixed_map_row(&fixture.player, &fixture.generator, std::string_view(row).substr(1)) == PARSE_ERROR_NONE);
    CHECK(fixture.y == 6);
    CHECK(fixture.x == 4);
}

TEST_CASE("QuestFixedMap rejects unsafe typed symbols before writing any output")
{
    for (const auto byte : { 0x00, 0x1f, 0x7f, 0x80, 0xfe, 0xff }) {
        for (const auto *source : { "base", "legend", "map" }) {
            CAPTURE(byte);
            CAPTURE(source);
            if (byte == 0 && std::string_view(source) == "map") {
                continue; // The direct application API retains NUL termination.
            }
            FixedMapFixture fixture;
            const auto restore = scoped_quest_layout_state();
            QuestType quest;
            QuestFixedMap map;
            map.legend['A'].grid.feature = 2;
            map.maps = { { "A" } };
            const auto symbol = static_cast<char>(byte);
            if (std::string_view(source) == "base") {
                QuestFixedMapList::get_instance().set_base_legend({ { symbol, {} } });
            } else if (std::string_view(source) == "legend") {
                map.legend[symbol] = {};
            } else {
                map.maps.push_back({ "A" + std::string(1, symbol) });
            }
            CHECK(generate_quest_floor_from_json(&fixture.player, quest, map) == PARSE_ERROR_INVALID_VALUE);
            CHECK(letter['A'].feature == 1);
            CHECK(fixture.floor.grid_array[0][0].feat == 0);
            CHECK(fixture.floor.height == 0);
            CHECK(fixture.player.oldpy == 0);
        }
    }
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
