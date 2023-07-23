#pragma once

#include "system/angband.h"

class FloorType;
struct grid_type;
class PlayerType;
enum class TerrainCharacteristics;
bool in_bounds(FloorType *floor_ptr, POSITION y, POSITION x);
bool in_bounds2(FloorType *floor_ptr, POSITION y, POSITION x);
bool in_bounds2u(FloorType *floor_ptr, POSITION y, POSITION x);
bool is_cave_empty_bold(PlayerType *player_ptr, POSITION x, POSITION y);
bool is_cave_empty_bold2(PlayerType *player_ptr, POSITION x, POSITION y);
bool cave_has_flag_bold(FloorType *floor_ptr, POSITION y, POSITION x, TerrainCharacteristics f_idx);
bool player_has_los_bold(PlayerType *player_ptr, POSITION y, POSITION x);
bool player_bold(PlayerType *player_ptr, POSITION y, POSITION x);
bool cave_stop_disintegration(FloorType *floor_ptr, POSITION y, POSITION x);
bool cave_los_bold(FloorType *floor_ptr, POSITION y, POSITION x);
bool feat_supports_los(FEAT_IDX f_idx);
bool cave_clean_bold(FloorType *floor_ptr, POSITION y, POSITION x);
bool cave_drop_bold(FloorType *floor_ptr, POSITION y, POSITION x);
bool pattern_tile(FloorType *floor_ptr, POSITION y, POSITION x);
