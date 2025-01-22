#include "grid/feature.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "grid/lighting-colors-table.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "player/special-defense-types.h"
#include "room/door-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h" // @todo 相互依存している. 後で何とかする.
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"
#include <span>

/*** Terrain feature variables ***/

/* Pattern */
FEAT_IDX feat_pattern_start;
FEAT_IDX feat_pattern_1;
FEAT_IDX feat_pattern_2;
FEAT_IDX feat_pattern_3;
FEAT_IDX feat_pattern_4;
FEAT_IDX feat_pattern_end;
FEAT_IDX feat_pattern_old;
FEAT_IDX feat_pattern_exit;
FEAT_IDX feat_pattern_corrupted;

FEAT_IDX feat_wall_outer;
FEAT_IDX feat_wall_inner;
FEAT_IDX feat_wall_solid;
FEAT_IDX feat_ground_type[100], feat_wall_type[100];

FEAT_IDX feat_locked_door_random(int door_type)
{
    const auto &terrains = TerrainList::get_instance();
    const auto &door = feat_door[door_type];
    std::span<const FEAT_IDX> candidates(std::begin(door.locked), door.num_locked);
    return candidates.empty() ? terrains.get_terrain_id(TerrainTag::NONE) : rand_choice(candidates);
}

FEAT_IDX feat_jammed_door_random(int door_type)
{
    const auto &terrains = TerrainList::get_instance();
    const auto &door = feat_door[door_type];
    std::span<const FEAT_IDX> candidates(std::begin(door.jammed), door.num_jammed);
    return candidates.empty() ? terrains.get_terrain_id(TerrainTag::NONE) : rand_choice(candidates);
}

void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag)
{
    cave_set_feat(player_ptr, pos.y, pos.x, TerrainList::get_instance().get_terrain_id(tag));
}

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(PlayerType *player_ptr, POSITION y, POSITION x, FEAT_IDX feat)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    const auto &dungeon = floor.get_dungeon_definition();
    if (!AngbandWorld::get_instance().character_dungeon) {
        grid.mimic = 0;
        grid.feat = feat;
        if (terrain.flags.has(TerrainCharacteristics::GLOW) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS)) {
            for (auto i = 0; i < 9; i++) {
                const Pos2D pos_neighbor(y + ddy_ddd[i], x + ddx_ddd[i]);
                if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
                    continue;
                }

                floor.get_grid(pos_neighbor).info |= CAVE_GLOW;
            }
        }

        return;
    }

    const auto old_los = floor.has_terrain_characteristics(pos, TerrainCharacteristics::LOS);
    const auto old_mirror = grid.is_mirror();
    grid.mimic = 0;
    grid.feat = feat;
    grid.info &= ~(CAVE_OBJECT);
    if (old_mirror && dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        grid.info &= ~(CAVE_GLOW);
        if (!view_torch_grids) {
            grid.info &= ~(CAVE_MARK);
        }

        update_local_illumination(player_ptr, y, x);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
        grid.info &= ~(CAVE_MARK);
    }

    if (grid.has_monster()) {
        update_monster(player_ptr, grid.m_idx, false);
    }

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
    if (old_los ^ terrain.flags.has(TerrainCharacteristics::LOS)) {
        static constexpr auto flags = {
            StatusRecalculatingFlag::VIEW,
            StatusRecalculatingFlag::LITE,
            StatusRecalculatingFlag::MONSTER_LITE,
            StatusRecalculatingFlag::MONSTER_STATUSES,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::GLOW) || dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (auto i = 0; i < 9; i++) {
        const Pos2D pos_neighbor(y + ddy_ddd[i], x + ddx_ddd[i]);
        if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
            continue;
        }

        auto &grid_neighbor = floor.get_grid(pos_neighbor);
        grid_neighbor.info |= CAVE_GLOW;
        if (grid_neighbor.is_view()) {
            if (grid_neighbor.has_monster()) {
                update_monster(player_ptr, grid_neighbor.m_idx, false);
            }

            note_spot(player_ptr, pos_neighbor.y, pos_neighbor.x);
            lite_spot(player_ptr, pos_neighbor.y, pos_neighbor.x);
        }

        update_local_illumination(player_ptr, pos_neighbor.y, pos_neighbor.x);
    }

    if (floor.get_grid(player_ptr->get_position()).info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}
