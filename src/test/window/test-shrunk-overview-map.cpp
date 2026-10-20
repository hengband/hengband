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
#include "window/overview-map-cache.h"
#include "window/shrunk-overview-map.h"
#include "world/world.h"
#include <array>
#include <doctest/doctest.h>
#include <memory>

namespace {
/*!
 * @brief 縮小マップの全てのマスが一致するか
 */
bool is_same_shrunk_map(const ShrunkOverviewMap &lhs, const ShrunkOverviewMap &rhs, int hgt, int wid)
{
    for (auto y = 0; y < hgt; y++) {
        for (auto x = 0; x < wid; x++) {
            if ((lhs.get_cell(y, x) != rhs.get_cell(y, x)) || (lhs.get_autopick(y, x) != rhs.get_autopick(y, x))) {
                return false;
            }
        }
    }

    return true;
}
}

TEST_CASE("ShrunkOverviewMap rebuilds only the cells around changed grids")
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

    // 床 (優先度 1)、壁 (優先度 2)、床と同じ優先度で記号の違う瓦礫の 3 つの地形を足す。どれも記憶していれば表示される
    terrains.resize(old_size + 3);
    const auto floor_id = static_cast<short>(old_size);
    const auto wall_id = static_cast<short>(old_size + 1);
    const auto rubble_id = static_cast<short>(old_size + 2);
    const auto define_terrain = [&terrains](short id, TerrainCharacteristics characteristic, char character, int16_t priority) {
        auto &terrain = terrains.get_terrain(id);
        terrain.idx = id;
        terrain.mimic = id;
        terrain.priority = priority;
        terrain.flags.clear();
        terrain.flags.set(characteristic).set(TerrainCharacteristics::REMEMBER);
        for (auto lighting = F_LIT_STANDARD; lighting < F_LIT_MAX; ++lighting) {
            terrain.symbol_configs[lighting] = { TERM_WHITE, character };
        }
    };
    define_terrain(floor_id, TerrainCharacteristics::LOS, '.', 1);
    define_terrain(wall_id, TerrainCharacteristics::WALL, '#', 2);
    define_terrain(rubble_id, TerrainCharacteristics::LOS, ':', 1);

    // 壁の表示と照明による色の変化がほかのグリッドやプレイヤーの位置に左右されないようにする
    view_hidden_walls = true;
    view_unsafe_walls = true;
    view_unsafe_grids = false;
    view_special_lite = false;
    view_granite_lite = false;
    world.set_wild_mode(false);
    rfu.reset_flag(MainWindowRedrawingFlag::MAP);

    // 9x8 のフロアを 3x3 に縮める (1 マスが 3x3 グリッドの区画になる。下の 1 行のマスは区画が 2 行しかない)
    auto floor_ptr = std::make_unique<FloorType>();
    auto &floor = *floor_ptr;
    floor.width = 9;
    floor.height = 8;
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

    constexpr auto hgt = 3;
    constexpr auto wid = 3;
    cache.mark_all_dirty();
    cache.update(player_ptr.get());
    ShrunkOverviewMap shrunk_map(floor.height, floor.width, hgt, wid);
    shrunk_map.update(cache);
    REQUIRE(shrunk_map.get_yrat() == 3);
    REQUIRE(shrunk_map.get_xrat() == 3);
    REQUIRE(shrunk_map.get_cell(1, 1).symbol.character == '.');

    const auto change_grid = [&](const Pos2D &pos, short feat) {
        floor.get_grid(pos).feat = feat;
        cache.mark_dirty(pos);
        cache.update(player_ptr.get());
        shrunk_map.update(cache);
    };

    SUBCASE("The grid with the highest priority in the block is shown")
    {
        change_grid({ 4, 5 }, wall_id);
        CHECK(shrunk_map.get_cell(1, 1).symbol.character == '#');
        CHECK(shrunk_map.get_cell(0, 1).symbol.character == '.');

        change_grid({ 4, 5 }, floor_id);
        CHECK(shrunk_map.get_cell(1, 1).symbol.character == '.');
    }

    SUBCASE("A change in a neighboring block can change the cell")
    {
        // 同じ優先度なら、周り 8 マスに同じ記号が 4 つ以下のグリッドの優先度を 1 上げて選ぶ。
        // 区画の右上の角 (3, 5) の周りのうち、区画の外の 5 マスを瓦礫にすると、床が 3 つになって優先度が上がる
        REQUIRE(shrunk_map.get_cell(1, 1).priority == 1);
        for (const auto &pos : std::array<Pos2D, 5>{ { { 2, 4 }, { 2, 5 }, { 2, 6 }, { 3, 6 }, { 4, 6 } } }) {
            change_grid(pos, rubble_id);
        }

        CHECK(shrunk_map.get_cell(1, 1).symbol.character == '.');
        CHECK(shrunk_map.get_cell(1, 1).priority == 2);
        ShrunkOverviewMap rebuilt(floor.height, floor.width, hgt, wid);
        rebuilt.update(cache);
        CHECK(is_same_shrunk_map(shrunk_map, rebuilt, hgt, wid));
    }

    SUBCASE("The result matches a full rebuild after many changes")
    {
        // 決まった順に地形を書き換え、毎回全部作り直したものと比べる。
        // 右端と下端の区画が狭くなる大きさ (9x8 を 3x2 に縮めると 1 マスが 3x5 グリッドで、右端の区画は幅 4) でも比べる
        constexpr auto uneven_hgt = 3;
        constexpr auto uneven_wid = 2;
        ShrunkOverviewMap uneven_map(floor.height, floor.width, uneven_hgt, uneven_wid);
        uneven_map.update(cache);
        REQUIRE(uneven_map.get_xrat() == 5);
        const std::array<short, 3> feats{ floor_id, wall_id, rubble_id };
        for (auto i = 0; i < 200; i++) {
            const Pos2D pos((i * 5) % floor.height, (i * 7) % floor.width);
            change_grid(pos, feats[(i * 11) % feats.size()]);
            uneven_map.update(cache);

            ShrunkOverviewMap rebuilt(floor.height, floor.width, hgt, wid);
            rebuilt.update(cache);
            REQUIRE(is_same_shrunk_map(shrunk_map, rebuilt, hgt, wid));
            ShrunkOverviewMap uneven_rebuilt(floor.height, floor.width, uneven_hgt, uneven_wid);
            uneven_rebuilt.update(cache);
            REQUIRE(is_same_shrunk_map(uneven_map, uneven_rebuilt, uneven_hgt, uneven_wid));
        }
    }

    SUBCASE("A new generation rebuilds all cells")
    {
        // 印を付けずに書き換えたグリッドは、全グリッドを求め直したときに初めて反映される
        floor.get_grid({ 0, 0 }).feat = wall_id;
        floor.get_grid({ 7, 8 }).feat = wall_id;
        cache.update(player_ptr.get());
        shrunk_map.update(cache);
        CHECK(shrunk_map.get_cell(0, 0).symbol.character == '.');

        cache.mark_all_dirty();
        cache.update(player_ptr.get());
        shrunk_map.update(cache);
        CHECK(shrunk_map.get_cell(0, 0).symbol.character == '#');
        CHECK(shrunk_map.get_cell(2, 2).symbol.character == '#');
    }

    SUBCASE("The cells outside the floor are blank")
    {
        // 縮小しない大きさにすると、フロアより右と下のマスはどのグリッドにも当たらない
        ShrunkOverviewMap larger(floor.height, floor.width, 10, 12);
        larger.update(cache);
        CHECK(larger.get_cell(7, 8).symbol.character == '.');
        CHECK(larger.get_cell(8, 0).symbol.character == ' ');
        CHECK(larger.get_cell(0, 9).symbol.character == ' ');
    }
}
