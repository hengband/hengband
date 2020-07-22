#include "grid/feature.h"
#include "grid/lighting-colors-table.h"
#include "util/bit-flags-calculator.h"

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
FEAT_IDX feat_glyph;
FEAT_IDX feat_explosive_rune;
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
	return have_flag(f_info[feat].flags, FF_TRAP);
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

	return (have_flag(f_ptr->flags, FF_OPEN) || have_flag(f_ptr->flags, FF_BASH)) &&
		!have_flag(f_ptr->flags, FF_MOVE);
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
