#include "game-option/map-screen-options.h"
#include "system/deferred-update-requests.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/term-color-types.h"
#include "test/scoped-restore.h"
#include "util/finalizer.h"
#include "view/display-map.h"
#include "window/overview-map-cache.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <memory>
#include <vector>

TEST_CASE("OverviewMapCache recomputes only the grids marked dirty")
{
    auto &terrains = TerrainList::get_instance();
    auto &world = AngbandWorld::get_instance();
    auto &rfu = DeferredUpdateRequests::get_instance();
    const auto old_size = terrains.size();
    const auto restore_options = test::scoped_restore(view_hidden_walls, view_unsafe_walls, view_unsafe_grids, view_special_lite, view_granite_lite, view_bright_lite);
    auto &cache = OverviewMapCache::get_instance();
    const auto restore = util::make_finalizer([&terrains, &world, &rfu, &cache, old_size, wild = world.is_wild_mode()] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
        rfu.reset_flag(MainWindowRedrawingFlag::MAP);

        // このテストのフロアから求めた結果や表示の変化を、ほかのテストで使わせない
        cache.mark_all_dirty();
        cache.reset_changed();
    });

    // 床と壁の地形を足す。どちらも記憶していれば表示される
    terrains.resize(old_size + 2);
    const auto floor_id = static_cast<short>(old_size);
    const auto wall_id = static_cast<short>(old_size + 1);
    const auto define_terrain = [&terrains](short id, TerrainCharacteristics characteristic, char character) {
        auto &terrain = terrains.get_terrain(id);
        terrain.idx = id;
        terrain.mimic = id;
        terrain.flags.clear();
        terrain.flags.set(characteristic).set(TerrainCharacteristics::REMEMBER);
        for (auto lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
            terrain.symbol_configs[lighting] = { TERM_WHITE, character };
        }
    };
    define_terrain(floor_id, TerrainCharacteristics::LOS, '.');
    define_terrain(wall_id, TerrainCharacteristics::WALL, '#');

    // 壁の表示と照明による色の変化がほかのグリッドやプレイヤーの位置に左右されないようにする
    view_hidden_walls = true;
    view_unsafe_walls = true;
    view_unsafe_grids = false;
    view_special_lite = false;
    view_granite_lite = false;
    world.set_wild_mode(false);
    rfu.reset_flag(MainWindowRedrawingFlag::MAP);

    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.width = 5;
    floor.height = 4;
    for (auto y = 0; y < MAX_HGT; y++) {
        for (auto x = 0; x < MAX_WID; x++) {
            auto &grid = floor.get_grid({ y, x });
            grid.feat = floor_id;
            grid.info = CAVE_MARK;
        }
    }

    // プレイヤーの記号は種族の表を引くので、プレイヤーはフロアの外に置く
    auto player_ptr = std::make_unique<PlayerType>();
    player_ptr->current_floor_ptr = floor_ptr.get();
    player_ptr->y = 100;
    player_ptr->x = 100;

    cache.mark_all_dirty();
    cache.update(player_ptr.get());
    REQUIRE(cache.get_grid({ 1, 2 }).symbol.character == '.');
    REQUIRE(cache.has_changed());
    cache.reset_changed();

    SUBCASE("Unmarked grids keep the previous result until marked")
    {
        floor.get_grid({ 1, 2 }).feat = wall_id;
        floor.get_grid({ 2, 3 }).feat = wall_id;
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 1, 2 }).symbol.character == '.');
        CHECK_FALSE(cache.has_changed());

        cache.mark_dirty({ 1, 2 });
        CHECK(cache.has_dirty_grids());
        cache.update(player_ptr.get());
        CHECK_FALSE(cache.has_dirty_grids());
        CHECK(cache.get_grid({ 1, 2 }).symbol.character == '#');
        CHECK(cache.get_grid({ 2, 3 }).symbol.character == '.');
    }

    SUBCASE("A change is remembered until reset_changed()")
    {
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK_FALSE(cache.has_changed());

        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK(cache.has_changed());

        // 変わらなかったグリッドを求め直しても、前の変化は忘れない
        cache.mark_dirty({ 2, 3 });
        cache.update(player_ptr.get());
        CHECK(cache.has_changed());

        cache.reset_changed();
        CHECK_FALSE(cache.has_changed());
    }

    SUBCASE("A priority change alone is a change")
    {
        // 記号は同じで優先度だけ違う地形にする (縮小するときに選ばれる記号が変わる)
        terrains.get_terrain(wall_id).symbol_configs = terrains.get_terrain(floor_id).symbol_configs;
        terrains.get_terrain(wall_id).priority = static_cast<int16_t>(terrains.get_terrain(floor_id).priority + 1);
        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK(cache.has_changed());
        CHECK(cache.get_grid({ 1, 2 }).symbol.character == '.');
    }

    SUBCASE("The lighting options are ignored and restored")
    {
        // 照明による記号の変化を使うと、記憶する地形 (view_granite_lite) と記憶しない地形 (view_special_lite) の
        // どちらも暗い記号になるようにする
        terrains.get_terrain(floor_id).symbol_configs[F_LIT_DARK] = { TERM_L_DARK, '.' };
        auto &unremembered = terrains.get_terrain(wall_id);
        unremembered.flags.reset(TerrainCharacteristics::REMEMBER);
        unremembered.symbol_configs[F_LIT_DARK] = { TERM_L_DARK, '#' };
        floor.get_grid({ 2, 3 }).feat = wall_id;
        view_special_lite = true;
        view_granite_lite = true;
        view_bright_lite = true;
        REQUIRE(map_info(player_ptr.get(), { 1, 2 }).symbol_foreground.color == TERM_L_DARK);
        REQUIRE(map_info(player_ptr.get(), { 2, 3 }).symbol_foreground.color == TERM_L_DARK);

        cache.mark_dirty({ 1, 2 });
        cache.mark_dirty({ 2, 3 });
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 1, 2 }).symbol.color == TERM_WHITE);
        CHECK(cache.get_grid({ 2, 3 }).symbol.color == TERM_WHITE);
        CHECK(view_special_lite);
        CHECK(view_granite_lite);
    }

    SUBCASE("All grids are recomputed after mark_all_dirty()")
    {
        floor.get_grid({ 0, 0 }).feat = wall_id;
        floor.get_grid({ 3, 4 }).feat = wall_id;
        cache.mark_all_dirty();
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 0, 0 }).symbol.character == '#');
        CHECK(cache.get_grid({ 3, 4 }).symbol.character == '#');
    }

    SUBCASE("A pending map redraw request recomputes all grids")
    {
        floor.get_grid({ 0, 4 }).feat = wall_id;
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 0, 4 }).symbol.character == '#');
    }

    SUBCASE("A floor size change rebuilds the cache")
    {
        floor.width = 7;
        floor.height = 6;
        floor.get_grid({ 5, 6 }).feat = wall_id;
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 5, 6 }).symbol.character == '#');
        CHECK(cache.get_grid({ 1, 2 }).symbol.character == '.');
    }

    SUBCASE("Changed grids are recorded in the current generation")
    {
        const auto generation = cache.get_generation();
        CHECK(cache.get_changed_positions().empty());

        // 求め直しても表示が変わらなければ記録しない
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK(cache.get_changed_positions().empty());

        floor.get_grid({ 1, 2 }).feat = wall_id;
        floor.get_grid({ 2, 3 }).feat = wall_id;
        cache.mark_dirty({ 2, 3 });
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK(cache.get_generation() == generation);
        const std::vector<Pos2D> expected{ { 2, 3 }, { 1, 2 } };
        CHECK(cache.get_changed_positions() == expected);

        // 全グリッドを求め直すと、記録に無いグリッドも変わったかもしれないので世代を変える
        cache.mark_all_dirty();
        cache.update(player_ptr.get());
        CHECK(cache.get_generation() != generation);
        CHECK(cache.get_changed_positions().empty());
    }

    SUBCASE("A long record of changes starts a new generation")
    {
        const auto generation = cache.get_generation();
        const auto grid_count = floor.width * floor.height;
        for (auto i = 0; i < grid_count; i++) {
            floor.get_grid({ 1, 2 }).feat = (i % 2 == 0) ? wall_id : floor_id;
            cache.mark_dirty({ 1, 2 });
            cache.update(player_ptr.get());
        }

        CHECK(cache.get_generation() == generation);
        CHECK(cache.get_changed_positions().size() == static_cast<size_t>(grid_count));

        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.mark_dirty({ 1, 2 });
        cache.update(player_ptr.get());
        CHECK(cache.get_generation() != generation);
        CHECK(cache.get_changed_positions().empty());
    }

    SUBCASE("Marking a grid outside the floor is ignored")
    {
        cache.mark_dirty({ -1, 0 });
        cache.mark_dirty({ 0, 5 });
        cache.mark_dirty({ 4, 0 });

        // 範囲を確かめずに添字を求めると、(0, 5) は (1, 0) と同じ添字になる
        floor.get_grid({ 1, 0 }).feat = wall_id;
        cache.update(player_ptr.get());
        CHECK(cache.get_grid({ 1, 0 }).symbol.character == '.');
    }
}
