#pragma once

class FloorType;
struct grid_type;
class PlayerType;
enum class TerrainCharacteristics;
bool in_bounds(FloorType *floor_ptr, int y, int x);
bool in_bounds2(FloorType *floor_ptr, int y, int x);
bool in_bounds2u(FloorType *floor_ptr, int y, int x);
bool is_cave_empty_bold(PlayerType *player_ptr, int x, int y);
bool is_cave_empty_bold2(PlayerType *player_ptr, int x, int y);
bool cave_has_flag_bold(const FloorType *floor_ptr, int y, int x, TerrainCharacteristics f_idx);
bool player_bold(PlayerType *player_ptr, int y, int x);
bool cave_stop_disintegration(FloorType *floor_ptr, int y, int x);
bool cave_los_bold(FloorType *floor_ptr, int y, int x);
bool feat_supports_los(short f_idx);
bool cave_clean_bold(FloorType *floor_ptr, int y, int x);
bool cave_drop_bold(FloorType *floor_ptr, int y, int x);
bool pattern_tile(FloorType *floor_ptr, int y, int x);
