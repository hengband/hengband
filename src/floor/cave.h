#pragma once

#include "system/angband.h"

struct floor_type;
struct grid_type;
class PlayerType;
enum class FloorFeatureType;
bool in_bounds(floor_type *floor_ptr, POSITION y, POSITION x);
bool in_bounds2(floor_type *floor_ptr, POSITION y, POSITION x);
bool in_bounds2u(floor_type *floor_ptr, POSITION y, POSITION x);
bool is_cave_empty_bold(PlayerType *player_ptr, POSITION x, POSITION y);
bool is_cave_empty_bold2(PlayerType *player_ptr, POSITION x, POSITION y);
bool cave_has_flag_bold(floor_type *floor_ptr, POSITION y, POSITION x, FloorFeatureType f_idx);
bool player_has_los_bold(PlayerType *player_ptr, POSITION y, POSITION x);
bool player_bold(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_stop_disintegration(floor_type *floor_ptr, POSITION y, POSITION x);
bool cave_los_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool feat_supports_los(FEAT_IDX f_idx);
bool cave_clean_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool cave_drop_bold(floor_type *floor_ptr, POSITION y, POSITION x);
bool pattern_tile(floor_type *floor_ptr, POSITION y, POSITION x);
