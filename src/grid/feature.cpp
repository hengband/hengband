﻿#include "grid/feature.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "grid/lighting-colors-table.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "player/special-defense-types.h"
#include "room/door-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*** Terrain feature variables ***/

/* The terrain feature arrays */
feature_type *f_info;
char *f_name;
char *f_tag;

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
 * Maximum number of terrain features in f_info.txt
 */
FEAT_IDX max_f_idx;

/*!
 * @brief 地形が罠持ちであるかの判定を行う。 / Return TRUE if the given feature is a trap
 * @param feat 地形情報のID
 * @return 罠持ちの地形ならばTRUEを返す。
 */
bool is_trap(player_type *player_ptr, FEAT_IDX feat)
{
	/* 関数ポインタの都合 */
	(void)player_ptr;
	return has_flag(f_info[feat].flags, FF_TRAP);
}

/*!
 * @brief 地形が閉じたドアであるかの判定を行う。 / Return TRUE if the given grid is a closed door
 * @param feat 地形情報のID
 * @return 閉じたドアのある地形ならばTRUEを返す。
 */
bool is_closed_door(player_type *player_ptr, FEAT_IDX feat)
{
	/* 関数ポインタの都合 */
	(void)player_ptr;
	feature_type *f_ptr = &f_info[feat];

	return (has_flag(f_ptr->flags, FF_OPEN) || has_flag(f_ptr->flags, FF_BASH)) &&
		!has_flag(f_ptr->flags, FF_MOVE);
}

/*!
 * @brief 調査中
 * @todo コメントを付加すること
 */
void apply_default_feat_lighting(TERM_COLOR *f_attr, SYMBOL_CODE *f_char)
{
    TERM_COLOR s_attr = f_attr[F_LIT_STANDARD];
    SYMBOL_CODE s_char = f_char[F_LIT_STANDARD];

    if (is_ascii_graphics(s_attr)) /* For ASCII */
    {
        f_attr[F_LIT_LITE] = lighting_colours[s_attr & 0x0f][0];
        f_attr[F_LIT_DARK] = lighting_colours[s_attr & 0x0f][1];
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_char[i] = s_char;
    } else /* For tile graphics */
    {
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_attr[i] = s_attr;
        f_char[F_LIT_LITE] = s_char + 2;
        f_char[F_LIT_DARK] = s_char + 1;
    }
}

/*
 * Not using graphical tiles for this feature?
 */
bool is_ascii_graphics(char x) { return (x & 0x80) == 0; }

/*
 * Determine if a "feature" is "permanent wall"
 */
bool permanent_wall(feature_type *f_ptr) { return has_flag(f_ptr->flags, FF_WALL) && has_flag(f_ptr->flags, FF_PERMANENT); }

FEAT_IDX feat_locked_door_random(int door_type)
{
    return feat_door[door_type].num_locked ? feat_door[door_type].locked[randint0(feat_door[door_type].num_locked)] : feat_none;
}

FEAT_IDX feat_jammed_door_random(int door_type)
{
    return feat_door[door_type].num_jammed ? feat_door[door_type].jammed[randint0(feat_door[door_type].num_jammed)] : feat_none;
}

/*
 * Determine if a "legal" grid is within "los" of the player *
 * Note the use of comparison to zero to force a "boolean" result
 */
static bool player_has_los_grid(grid_type *g_ptr) { return (g_ptr->info & CAVE_VIEW) != 0; }

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void cave_set_feat(player_type *player_ptr, POSITION y, POSITION x, FEAT_IDX feat)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[feat];
    if (!current_world_ptr->character_dungeon) {
        g_ptr->mimic = 0;
        g_ptr->feat = feat;
        if (has_flag(f_ptr->flags, FF_GLOW) && !(d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS)) {
            for (DIRECTION i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                if (!in_bounds2(floor_ptr, yy, xx))
                    continue;

                floor_ptr->grid_array[yy][xx].info |= CAVE_GLOW;
            }
        }

        return;
    }

    bool old_los = cave_has_flag_bold(floor_ptr, y, x, FF_LOS);
    bool old_mirror = is_mirror_grid(g_ptr);

    g_ptr->mimic = 0;
    g_ptr->feat = feat;
    g_ptr->info &= ~(CAVE_OBJECT);
    if (old_mirror && (d_info[floor_ptr->dungeon_idx].flags1 & DF1_DARKNESS)) {
        g_ptr->info &= ~(CAVE_GLOW);
        if (!view_torch_grids)
            g_ptr->info &= ~(CAVE_MARK);

        update_local_illumination(player_ptr, y, x);
    }

    if (!has_flag(f_ptr->flags, FF_REMEMBER))
        g_ptr->info &= ~(CAVE_MARK);
    if (g_ptr->m_idx)
        update_monster(player_ptr, g_ptr->m_idx, FALSE);

    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
    if (old_los ^ has_flag(f_ptr->flags, FF_LOS))
        player_ptr->update |= PU_VIEW | PU_LITE | PU_MON_LITE | PU_MONSTERS;

    if (!has_flag(f_ptr->flags, FF_GLOW) || (d_info[player_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
        return;

    for (DIRECTION i = 0; i < 9; i++) {
        POSITION yy = y + ddy_ddd[i];
        POSITION xx = x + ddx_ddd[i];
        if (!in_bounds2(floor_ptr, yy, xx))
            continue;

        grid_type *cc_ptr;
        cc_ptr = &floor_ptr->grid_array[yy][xx];
        cc_ptr->info |= CAVE_GLOW;

        if (player_has_los_grid(cc_ptr)) {
            if (cc_ptr->m_idx)
                update_monster(player_ptr, cc_ptr->m_idx, FALSE);
            note_spot(player_ptr, yy, xx);
            lite_spot(player_ptr, yy, xx);
        }

        update_local_illumination(player_ptr, yy, xx);
    }

    if (player_ptr->special_defense & NINJA_S_STEALTH) {
        if (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW)
            set_superstealth(player_ptr, FALSE);
    }
}

FEAT_IDX conv_dungeon_feat(floor_type *floor_ptr, FEAT_IDX newfeat)
{
    feature_type *f_ptr = &f_info[newfeat];
    if (!has_flag(f_ptr->flags, FF_CONVERT))
        return newfeat;

    switch (f_ptr->subtype) {
    case CONVERT_TYPE_FLOOR:
        return feat_ground_type[randint0(100)];
    case CONVERT_TYPE_WALL:
        return feat_wall_type[randint0(100)];
    case CONVERT_TYPE_INNER:
        return feat_wall_inner;
    case CONVERT_TYPE_OUTER:
        return feat_wall_outer;
    case CONVERT_TYPE_SOLID:
        return feat_wall_solid;
    case CONVERT_TYPE_STREAM1:
        return d_info[floor_ptr->dungeon_idx].stream1;
    case CONVERT_TYPE_STREAM2:
        return d_info[floor_ptr->dungeon_idx].stream2;
    default:
        return newfeat;
    }
}
