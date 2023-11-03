#pragma once

#include "system/angband.h"
#include <string>
#include <vector>

/* Lighting levels of features' attr and char */
#define F_LIT_STANDARD 0 /* Standard */
#define F_LIT_LITE 1 /* Brightly lit */
#define F_LIT_DARK 2 /* Darkened */

#define F_LIT_NS_BEGIN 1 /* Nonstandard */

/*** Terrain feature variables ***/
extern FEAT_IDX feat_none;
extern FEAT_IDX feat_floor;
extern FEAT_IDX feat_rune_protection;
extern FEAT_IDX feat_rune_explosion;
extern FEAT_IDX feat_mirror;
extern FEAT_IDX feat_up_stair;
extern FEAT_IDX feat_down_stair;
extern FEAT_IDX feat_entrance;
extern FEAT_IDX feat_trap_open;
extern FEAT_IDX feat_trap_armageddon;
extern FEAT_IDX feat_trap_piranha;
extern FEAT_IDX feat_rubble;
extern FEAT_IDX feat_magma_vein;
extern FEAT_IDX feat_quartz_vein;
extern FEAT_IDX feat_granite;
extern FEAT_IDX feat_permanent;
extern FEAT_IDX feat_glass_floor;
extern FEAT_IDX feat_glass_wall;
extern FEAT_IDX feat_permanent_glass_wall;
extern FEAT_IDX feat_pattern_start;
extern FEAT_IDX feat_pattern_1;
extern FEAT_IDX feat_pattern_2;
extern FEAT_IDX feat_pattern_3;
extern FEAT_IDX feat_pattern_4;
extern FEAT_IDX feat_pattern_end;
extern FEAT_IDX feat_pattern_old;
extern FEAT_IDX feat_pattern_exit;
extern FEAT_IDX feat_pattern_corrupted;
extern FEAT_IDX feat_black_market;
extern FEAT_IDX feat_town;
extern FEAT_IDX feat_deep_water;
extern FEAT_IDX feat_shallow_water;
extern FEAT_IDX feat_deep_lava;
extern FEAT_IDX feat_shallow_lava;
extern FEAT_IDX feat_heavy_cold_zone;
extern FEAT_IDX feat_cold_zone;
extern FEAT_IDX feat_heavy_electrical_zone;
extern FEAT_IDX feat_electrical_zone;
extern FEAT_IDX feat_deep_acid_puddle;
extern FEAT_IDX feat_shallow_acid_puddle;
extern FEAT_IDX feat_deep_poisonous_puddle;
extern FEAT_IDX feat_shallow_poisonous_puddle;
extern FEAT_IDX feat_dirt;
extern FEAT_IDX feat_grass;
extern FEAT_IDX feat_flower;
extern FEAT_IDX feat_brake;
extern FEAT_IDX feat_tree;
extern FEAT_IDX feat_mountain;
extern FEAT_IDX feat_swamp;
extern FEAT_IDX feat_undetected;

extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100];
extern FEAT_IDX feat_wall_type[100];

class FloorType;
class PlayerType;
bool is_closed_door(PlayerType *player_ptr, FEAT_IDX feat);
bool is_trap(PlayerType *player_ptr, FEAT_IDX feat);
void apply_default_feat_lighting(TERM_COLOR *f_attr, char *f_char);
bool is_ascii_graphics(char x);
FEAT_IDX feat_locked_door_random(int door_type);
FEAT_IDX feat_jammed_door_random(int door_type);
void cave_set_feat(PlayerType *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
FEAT_IDX conv_dungeon_feat(const FloorType *floor_ptr, FEAT_IDX newfeat);
