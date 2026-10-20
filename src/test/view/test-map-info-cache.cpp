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
#include "view/map-info-cache.h"
#include "world/world.h"
#include <doctest/doctest.h>
#include <memory>

TEST_CASE("MapInfoCache recomputes only the grids invalidated")
{
    auto &terrains = TerrainList::get_instance();
    auto &world = AngbandWorld::get_instance();
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto old_size = terrains.size();
    const auto restore_options = test::scoped_restore(view_hidden_walls, view_unsafe_walls, view_unsafe_grids, view_special_lite, view_granite_lite, display_autopick);
    auto &cache = MapInfoCache::get_instance();
    const auto restore = util::make_finalizer([&terrains, &world, &dur, &cache, old_size, wild = world.is_wild_mode(), has_map = dur.has(MainWindowRedrawingFlag::MAP)] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
        if (has_map) {
            dur.set_flag(MainWindowRedrawingFlag::MAP);
        } else {
            dur.reset_flag(MainWindowRedrawingFlag::MAP);
        }

        // このテストのフロアから求めた結果を、ほかのテストで使わせない
        cache.invalidate_all();
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
    dur.reset_flag(MainWindowRedrawingFlag::MAP);
    display_autopick.clear();

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

    const auto character_at = [&cache, &player_ptr](const Pos2D &pos) {
        return cache.get(player_ptr.get(), pos).symbol_foreground.character;
    };
    cache.prepare(*player_ptr);
    REQUIRE(character_at({ 1, 2 }) == '.');

    SUBCASE("Grids keep the previous result until invalidated")
    {
        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '.');

        cache.invalidate({ 1, 2 });
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
    }

    SUBCASE("A stored result is returned without recomputing")
    {
        CHECK(cache.store({ 1, 2 }, { { TERM_WHITE, 'X' }, {} }));
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == 'X');
    }

    SUBCASE("store() reports whether the stored result differs from a valid one")
    {
        const auto symbol_pair = cache.get(player_ptr.get(), { 1, 2 });
        CHECK_FALSE(cache.store({ 1, 2 }, symbol_pair));
        CHECK(cache.store({ 1, 2 }, { { TERM_WHITE, 'X' }, {} }));
        cache.invalidate({ 1, 2 });
        CHECK(cache.store({ 1, 2 }, { { TERM_WHITE, 'X' }, {} }));
    }

    SUBCASE("All grids are recomputed after invalidate_all()")
    {
        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.invalidate_all();
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
    }

    SUBCASE("A pending map redraw request recomputes all grids")
    {
        floor.get_grid({ 1, 2 }).feat = wall_id;
        dur.set_flag(MainWindowRedrawingFlag::MAP);
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
    }

    SUBCASE("Results are neither used nor stored while displaying autopick targets")
    {
        display_autopick.set(AutopickMethod::ITEM_DISPLAY);
        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
        CHECK_FALSE(cache.store({ 1, 2 }, { { TERM_WHITE, 'X' }, {} }));

        // 表示中に覚えようとしたグリッドは、覚えていた結果も使わずに求め直す
        display_autopick.clear();
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
    }

    SUBCASE("A floor size change rebuilds the cache")
    {
        floor.width = 7;
        floor.height = 6;
        floor.get_grid({ 1, 2 }).feat = wall_id;
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 2 }) == '#');
    }

    SUBCASE("Grids outside the floor are ignored")
    {
        // 範囲を確かめずに添字を求めると、(0, 5) は (1, 0) と同じ添字になる
        REQUIRE(character_at({ 1, 0 }) == '.');
        floor.get_grid({ 1, 0 }).feat = wall_id;
        cache.store({ 0, 5 }, { { TERM_WHITE, 'X' }, {} });
        cache.invalidate({ 0, 5 });
        cache.prepare(*player_ptr);
        CHECK(character_at({ 1, 0 }) == '.');
    }
}
