#include "game-option/map-screen-options.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/term-color-types.h"
#include "test/scoped-restore.h"
#include "timed-effect/timed-effects.h"
#include "util/finalizer.h"
#include "view/display-map.h"
#include "world/world.h"
#include <doctest/doctest.h>

TEST_CASE("Map terrain observations do not disclose stale knowledge or darkness")
{
    auto &terrains = TerrainList::get_instance();
    auto &world = AngbandWorld::get_instance();
    const auto old_size = terrains.size();
    const auto restore_options = test::scoped_restore(view_hidden_walls, view_special_lite, view_yellow_lite, view_bright_lite, view_granite_lite);
    const auto restore = util::make_finalizer([&terrains, &world, old_size, wild = world.is_wild_mode()] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
    });
    terrains.resize(old_size + 2);
    const auto floor_id = static_cast<short>(old_size);
    const auto wall_id = static_cast<short>(old_size + 1);
    auto &plain = terrains.get_terrain(floor_id);
    plain.idx = floor_id;
    plain.mimic = floor_id;
    plain.flags.clear();
    plain.flags.set(TerrainCharacteristics::FLOOR).set(TerrainCharacteristics::LOS).set(TerrainCharacteristics::PROJECTION);
    auto &wall = terrains.get_terrain(wall_id);
    wall.idx = wall_id;
    wall.mimic = wall_id;
    wall.flags.clear();
    wall.flags.set(TerrainCharacteristics::WALL).set(TerrainCharacteristics::REMEMBER);
    world.set_wild_mode(false);
    view_hidden_walls = false;
    auto floor_ptr = std::make_unique<FloorType>();
    floor_ptr->width = 5;
    floor_ptr->height = 5;
    auto &floor = *floor_ptr;
    auto player_ptr = std::make_unique<PlayerType>();
    auto &player = *player_ptr;
    player.current_floor_ptr = floor_ptr.get();
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        grid.feat = floor_id;
        grid.mimic = 0;
        grid.info = 0;
    }
    const Pos2D pos{ 2, 2 };
    auto &grid = floor.get_grid(pos);
    // 全マスを、覚えている壁にする
    const auto fill_with_marked_walls = [&floor, wall_id] {
        for (const auto &pos_fill : floor.get_area()) {
            auto &grid_fill = floor.get_grid(pos_fill);
            grid_fill.feat = wall_id;
            grid_fill.info = CAVE_MARK;
        }
    };

    SUBCASE("Observation, leaving torchlight, and observing again")
    {
        grid.info = CAVE_VIEW | CAVE_LITE | CAVE_KNOWN;
        CHECK(is_map_terrain_visible(player, pos));
        grid.info = CAVE_KNOWN;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        grid.info |= CAVE_MARK;
        CHECK(is_map_terrain_visible(player, pos));
        grid.info &= ~CAVE_MARK;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        grid.info |= CAVE_VIEW | CAVE_LITE;
        CHECK(is_map_terrain_visible(player, pos));
    }
    SUBCASE("Unseen wall removal does not reveal a new passage")
    {
        grid.feat = wall_id;
        grid.info = CAVE_MARK | CAVE_KNOWN;
        CHECK(is_map_terrain_visible(player, pos));
        grid.feat = floor_id;
        grid.info &= ~CAVE_MARK;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
    }
    SUBCASE("Monster darkness, noctovision and blindness follow map rendering")
    {
        grid.info = CAVE_MARK | CAVE_KNOWN | CAVE_VIEW | CAVE_MNDK;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        player.see_nocto = 1;
        CHECK(is_map_terrain_visible(player, pos));
        player.effects()->blindness().set(10);
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        player.effects()->blindness().set(0);
        player.see_nocto = 0;
        grid.info |= CAVE_LITE;
        CHECK(is_map_terrain_visible(player, pos));
        grid.info &= ~CAVE_LITE;
        world.set_wild_mode(true);
        CHECK(is_map_terrain_visible(player, pos));
    }
    SUBCASE("Remembered transparent terrain is hidden by darkness but walls remain")
    {
        plain.flags.set(TerrainCharacteristics::REMEMBER);
        grid.info = CAVE_MARK | CAVE_VIEW | CAVE_MNDK;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        player.effects()->blindness().set(10);
        CHECK(is_map_terrain_visible(player, pos));
        player.effects()->blindness().set(0);
        grid.feat = wall_id;
        CHECK(is_map_terrain_visible(player, pos));
        for (const auto &neighbor : floor.get_area()) {
            floor.get_grid(neighbor).feat = wall_id;
        }
        CHECK_FALSE(is_map_terrain_visible(player, pos));
    }
    SUBCASE("Walls next to the outer wall count only open grids inside it")
    {
        fill_with_marked_walls();
        // 外周より内側のマスで、外周に接しているもの
        const Pos2D pos_edge{ 1, 1 };
        CHECK_FALSE(is_map_terrain_visible(player, pos_edge));
        // 外周のマスが開いていても見せない
        floor.get_grid({ 0, 0 }).feat = floor_id;
        floor.get_grid({ 0, 2 }).feat = floor_id;
        floor.get_grid({ 2, 0 }).feat = floor_id;
        CHECK_FALSE(is_map_terrain_visible(player, pos_edge));
        // 外周より内側のマスが開いていれば見せる
        floor.get_grid({ 2, 2 }).feat = floor_id;
        CHECK(is_map_terrain_visible(player, pos_edge));
        // 外周のマス自体の判定でも、フロアの外と外周のマスは開いているとみなさない
        floor.get_grid({ 2, 2 }).feat = wall_id;
        CHECK_FALSE(is_map_terrain_visible(player, { 0, 1 }));
        floor.get_grid({ 1, 2 }).feat = floor_id;
        CHECK(is_map_terrain_visible(player, { 0, 1 }));
    }
    SUBCASE("Walls on both sides of the inner area boundary count the right neighbours")
    {
        fill_with_marked_walls();
        // 外周から 2 マス以上内側のマス (5x5 では中央だけ)
        const Pos2D pos_inner{ 2, 2 };
        CHECK_FALSE(is_map_terrain_visible(player, pos_inner));
        floor.get_grid({ 1, 1 }).feat = floor_id;
        CHECK(is_map_terrain_visible(player, pos_inner));
        floor.get_grid({ 1, 1 }).feat = wall_id;
        // 右下で外周に接しているマスは、開いている外周のマスを数えない
        const Pos2D pos_edge{ 3, 3 };
        for (const auto &pos_outer : { Pos2D{ 4, 2 }, Pos2D{ 4, 3 }, Pos2D{ 4, 4 }, Pos2D{ 3, 4 }, Pos2D{ 2, 4 } }) {
            floor.get_grid(pos_outer).feat = floor_id;
        }
        CHECK_FALSE(is_map_terrain_visible(player, pos_edge));
        floor.get_grid({ 2, 2 }).feat = floor_id;
        CHECK(is_map_terrain_visible(player, pos_edge));
    }
    SUBCASE("Terrain lighting follows the map's lit / torch-lit / dark variants")
    {
        plain.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '.' };
        plain.symbol_configs[F_LIT_LITE] = { TERM_YELLOW, '.' };
        plain.symbol_configs[F_LIT_DARK] = { TERM_SLATE, '.' };
        view_special_lite = true;
        view_yellow_lite = true;
        view_bright_lite = true;
        view_granite_lite = true;
        grid.info = CAVE_MARK | CAVE_VIEW | CAVE_GLOW;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        grid.info |= CAVE_LITE;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_LITE);
        view_yellow_lite = false;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        view_yellow_lite = true;
        grid.info = CAVE_MARK | CAVE_GLOW;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_DARK);
        view_bright_lite = false;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        view_special_lite = false;
        grid.info = CAVE_MARK;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        view_special_lite = true;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_DARK);
        grid.info = 0;
        CHECK_FALSE(is_map_terrain_visible(player, pos));
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
    }
    SUBCASE("Wilderness lighting follows the time of day and blindness")
    {
        const auto restore_turn = test::scoped_restore(world.game_turn);
        constexpr auto turns_per_day = TURNS_PER_TICK * TOWN_DAWN;
        plain.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '.' };
        plain.symbol_configs[F_LIT_DARK] = { TERM_SLATE, '.' };
        wall.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '#' };
        wall.symbol_configs[F_LIT_DARK] = { TERM_SLATE, '#' };
        view_special_lite = true;
        view_granite_lite = true;
        world.set_wild_mode(true);
        grid.info = CAVE_MARK;
        world.game_turn = 0;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        world.game_turn = turns_per_day / 2;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_DARK);

        grid.feat = wall_id;
        world.game_turn = 0;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        player.effects()->blindness().set(10);
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_DARK);
        player.effects()->blindness().set(0);
        world.game_turn = turns_per_day / 2;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_DARK);
    }
    SUBCASE("Indistinguishable lighting symbols do not expose internal illumination")
    {
        view_special_lite = true;
        view_yellow_lite = true;
        view_bright_lite = true;
        for (auto lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
            plain.symbol_configs[lighting] = { TERM_L_BLUE, '~' };
        }
        for (const auto info : { CAVE_MARK | CAVE_VIEW | CAVE_GLOW, CAVE_MARK | CAVE_VIEW | CAVE_LITE, CAVE_MARK }) {
            grid.info = info;
            CHECK(is_map_terrain_visible(player, pos));
            CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_STANDARD);
        }

        plain.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '.' };
        grid.info = CAVE_MARK;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_LITE);
        grid.info = CAVE_MARK | CAVE_VIEW | CAVE_LITE;
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_LITE);
    }
    SUBCASE("Monochrome display hides lighting colours but preserves distinct characters")
    {
        view_special_lite = true;
        view_yellow_lite = true;
        grid.info = CAVE_MARK | CAVE_VIEW | CAVE_LITE;
        plain.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '.' };
        plain.symbol_configs[F_LIT_LITE] = { TERM_YELLOW, '.' };
        CHECK(decide_map_terrain_lighting(player, pos) == F_LIT_LITE);
        CHECK(decide_map_terrain_lighting(player, pos, TERM_WHITE) == F_LIT_STANDARD);
        plain.symbol_configs[F_LIT_LITE].character = '*';
        CHECK(decide_map_terrain_lighting(player, pos, TERM_WHITE) == F_LIT_LITE);
        CHECK(decide_map_terrain_lighting(player, pos, TERM_DARK) == F_LIT_STANDARD);
    }
    SUBCASE("Lighting of a grid known to be visible matches the full lighting decision")
    {
        plain.symbol_configs[F_LIT_STANDARD] = { TERM_WHITE, '.' };
        plain.symbol_configs[F_LIT_LITE] = { TERM_YELLOW, '.' };
        plain.symbol_configs[F_LIT_DARK] = { TERM_SLATE, '.' };
        view_special_lite = true;
        view_yellow_lite = true;
        view_bright_lite = true;
        view_granite_lite = true;
        const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);
        for (const auto info : { CAVE_MARK | CAVE_VIEW | CAVE_GLOW, CAVE_MARK | CAVE_VIEW | CAVE_LITE, CAVE_MARK | CAVE_GLOW, CAVE_MARK }) {
            grid.info = info;
            const MapVisibilityContext context(player);
            REQUIRE(is_map_terrain_visible(context, grid, terrain, pos));
            for (const auto monochrome : { tl::optional<uint8_t>{}, tl::optional<uint8_t>{ TERM_WHITE }, tl::optional<uint8_t>{ TERM_DARK } }) {
                CHECK(decide_visible_map_terrain_lighting(context, grid, terrain, pos, monochrome) == decide_map_terrain_lighting(player, pos, monochrome));
            }
        }
    }
}
