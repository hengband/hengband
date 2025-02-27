#pragma once

class FloorType;
bool cave_stop_disintegration(const FloorType &floor, int y, int x);
bool cave_los_bold(const FloorType &floor, int y, int x);
bool cave_clean_bold(const FloorType &floor, int y, int x);
bool cave_drop_bold(const FloorType &floor, int y, int x);
bool pattern_tile(const FloorType &floor, int y, int x);
