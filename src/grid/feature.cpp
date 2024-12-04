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
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h" // @todo 相互依存している. 後で何とかする.
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"
#include <span>

/*** Terrain feature variables ***/

/* Nothing */
FEAT_IDX feat_none;

/* Floor */
FEAT_IDX feat_floor;

/* Objects */
FEAT_IDX feat_rune_protection;
FEAT_IDX feat_rune_explosion;
FEAT_IDX feat_mirror;

/* Stairs */
FEAT_IDX feat_up_stair;
FEAT_IDX feat_down_stair;
FEAT_IDX feat_entrance;

/* Special traps */
FEAT_IDX feat_trap_open;
FEAT_IDX feat_trap_armageddon;
FEAT_IDX feat_trap_piranha;

/* Rubble */
FEAT_IDX feat_rubble;

/* Seams */
FEAT_IDX feat_magma_vein;
FEAT_IDX feat_quartz_vein;

/* Walls */
FEAT_IDX feat_granite;
FEAT_IDX feat_permanent;

/* Glass floor */
FEAT_IDX feat_glass_floor;

/* Glass walls */
FEAT_IDX feat_glass_wall;
FEAT_IDX feat_permanent_glass_wall;

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

/* Various */
FEAT_IDX feat_black_market;
FEAT_IDX feat_town;

/* Terrains */
FEAT_IDX feat_deep_water;
FEAT_IDX feat_shallow_water;
FEAT_IDX feat_deep_lava;
FEAT_IDX feat_shallow_lava;
FEAT_IDX feat_heavy_cold_zone;
FEAT_IDX feat_cold_zone;
FEAT_IDX feat_heavy_electrical_zone;
FEAT_IDX feat_electrical_zone;
FEAT_IDX feat_deep_acid_puddle;
FEAT_IDX feat_shallow_acid_puddle;
FEAT_IDX feat_deep_poisonous_puddle;
FEAT_IDX feat_shallow_poisonous_puddle;
FEAT_IDX feat_dirt;
FEAT_IDX feat_grass;
FEAT_IDX feat_flower;
FEAT_IDX feat_brake;
FEAT_IDX feat_tree;
FEAT_IDX feat_mountain;
FEAT_IDX feat_swamp;

/* Unknown grid (not detected) */
FEAT_IDX feat_undetected;

FEAT_IDX feat_wall_outer;
FEAT_IDX feat_wall_inner;
FEAT_IDX feat_wall_solid;
FEAT_IDX feat_ground_type[100], feat_wall_type[100];

/*
 * Not using graphical tiles for this feature?
 */
bool is_ascii_graphics(char x)
{
    return (x & 0x80) == 0;
}

FEAT_IDX feat_locked_door_random(int door_type)
{
    const auto &door = feat_door[door_type];
    std::span<const FEAT_IDX> candidates(std::begin(door.locked), door.num_locked);
    return candidates.empty() ? feat_none : rand_choice(candidates);
}

FEAT_IDX feat_jammed_door_random(int door_type)
{
    const auto &door = feat_door[door_type];
    std::span<const FEAT_IDX> candidates(std::begin(door.jammed), door.num_jammed);
    return candidates.empty() ? feat_none : rand_choice(candidates);
}

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(PlayerType *player_ptr, POSITION y, POSITION x, FEAT_IDX feat)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    const auto &dungeon = floor_ptr->get_dungeon_definition();
    if (!AngbandWorld::get_instance().character_dungeon) {
        g_ptr->mimic = 0;
        g_ptr->feat = feat;
        if (terrain.flags.has(TerrainCharacteristics::GLOW) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS)) {
            for (DIRECTION i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx)) {
                    continue;
                }

                floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
            }
        }

        return;
    }

    bool old_los = cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::LOS);
    bool old_mirror = g_ptr->is_mirror();

    g_ptr->mimic = 0;
    g_ptr->feat = feat;
    g_ptr->info &= ~(CAVE_OBJECT);
    if (old_mirror && dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        g_ptr->info &= ~(CAVE_GLOW);
        if (!view_torch_grids) {
            g_ptr->info &= ~(CAVE_MARK);
        }

        update_local_illumination(player_ptr, y, x);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
        g_ptr->info &= ~(CAVE_MARK);
    }

    if (g_ptr->has_monster()) {
        update_monster(player_ptr, g_ptr->m_idx, false);
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
        POSITION yy = y + ddy_ddd[i];
        POSITION xx = x + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, yy, xx)) {
            continue;
        }

        auto *cc_ptr = &floor_ptr->grid_array[yy][xx];
        cc_ptr->info |= CAVE_GLOW;
        if (cc_ptr->is_view()) {
            if (cc_ptr->has_monster()) {
                update_monster(player_ptr, cc_ptr->m_idx, false);
            }

            note_spot(player_ptr, yy, xx);
            lite_spot(player_ptr, yy, xx);
        }

        update_local_illumination(player_ptr, yy, xx);
    }

    if (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}
