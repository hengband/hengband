#pragma once

class FloorType;
class PlayerType;
bool in_bounds2(const FloorType &floor, int y, int x);
bool in_bounds2u(const FloorType &floor, int y, int x);
bool is_cave_empty_bold(PlayerType *player_ptr, int x, int y);
bool is_cave_empty_bold2(PlayerType *player_ptr, int x, int y);
bool cave_stop_disintegration(const FloorType &floor, int y, int x);
bool cave_los_bold(const FloorType &floor, int y, int x);
bool cave_clean_bold(const FloorType &floor, int y, int x);
bool cave_drop_bold(const FloorType &floor, int y, int x);
bool pattern_tile(const FloorType &floor, int y, int x);
