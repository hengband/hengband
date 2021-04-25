#pragma once

#include "system/angband.h"

enum feature_flag_type : int;
typedef struct floor_type floor_type;
typedef struct grid_type grid_type;
typedef struct player_type player_type;
bool in_bounds(floor_type *floor_ptr, POSITION y, POSITION x);
bool in_bounds2(floor_type *floor_ptr, POSITION y, POSITION x);
bool in_bounds2u(floor_type *floor_ptr, POSITION y, POSITION x);
bool is_cave_empty_bold(player_type *player_ptr, POSITION x, POSITION y);
bool is_cave_empty_bold2(player_type *player_ptr, POSITION x, POSITION y);
bool cave_has_flag_bold(floor_type *floor_ptr, POSITION y, POSITION x, feature_flag_type f_idx);
bool player_has_los_bold(player_type *player_ptr, POSITION y, POSITION x);
bool player_bold(player_type *player_ptr, POSITION y, POSITION x);
bool cave_stop_disintegration(floor_type *floor_ptr, POSITION y, POSITION x);
bool cave_los_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool feat_supports_los(FEAT_IDX f_idx);
bool cave_los_grid(grid_type *grid_ptr);
bool cave_has_flag_grid(grid_type *grid_ptr, int feature_flags);
bool cave_clean_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool cave_drop_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool pattern_tile(floor_type *floor_ptr, POSITION y, POSITION x);
