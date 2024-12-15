#pragma once

class FloorType;
class PlayerType;
enum class TerrainCharacteristics;
bool in_bounds(const FloorType *floor_ptr, int y, int x);
bool in_bounds2(const FloorType *floor_ptr, int y, int x);
bool in_bounds2u(const FloorType *floor_ptr, int y, int x);
bool is_cave_empty_bold(PlayerType *player_ptr, int x, int y);
bool is_cave_empty_bold2(PlayerType *player_ptr, int x, int y);
bool cave_has_flag_bold(const FloorType *floor_ptr, int y, int x, TerrainCharacteristics f_idx);
bool cave_stop_disintegration(const FloorType *floor_ptr, int y, int x);
bool cave_los_bold(const FloorType *floor_ptr, int y, int x);
bool cave_clean_bold(const FloorType *floor_ptr, int y, int x);
bool cave_drop_bold(const FloorType *floor_ptr, int y, int x);
bool pattern_tile(const FloorType *floor_ptr, int y, int x);
