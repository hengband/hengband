#include "game-option/map-screen-options.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
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
    const auto restore = util::make_finalizer([&terrains, &world, old_size, wild = world.is_wild_mode(), hidden = view_hidden_walls] {
        terrains.resize(old_size);
        world.set_wild_mode(wild);
        view_hidden_walls = hidden;
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
}
