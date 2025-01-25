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

/*** Terrain feature variables ***/
short feat_wall_outer;
short feat_wall_inner;
short feat_wall_solid;
short feat_ground_type[100], feat_wall_type[100];

void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag)
{
    cave_set_feat(player_ptr, pos, TerrainList::get_instance().get_terrain_id(tag));
}

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, short terrain_id)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    const auto &terrain = TerrainList::get_instance().get_terrain(terrain_id);
    const auto &dungeon = floor.get_dungeon_definition();
    if (!AngbandWorld::get_instance().character_dungeon) {
        grid.set_terrain_id(terrain_id);
        grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
        if (terrain.flags.has(TerrainCharacteristics::GLOW) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS)) {
            for (auto i = 0; i < 9; i++) {
                const auto pos_neighbor = pos + Pos2DVec(ddy_ddd[i], ddx_ddd[i]);
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
    grid.set_terrain_id(terrain_id);
    grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
    grid.info &= ~(CAVE_OBJECT);
    if (old_mirror && dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        grid.info &= ~(CAVE_GLOW);
        if (!view_torch_grids) {
            grid.info &= ~(CAVE_MARK);
        }

        update_local_illumination(player_ptr, pos.y, pos.x);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
        grid.info &= ~(CAVE_MARK);
    }

    if (grid.has_monster()) {
        update_monster(player_ptr, grid.m_idx, false);
    }

    note_spot(player_ptr, pos.y, pos.x);
    lite_spot(player_ptr, pos.y, pos.x);
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
        const auto pos_neighbor = pos + Pos2DVec(ddy_ddd[i], ddx_ddd[i]);
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
