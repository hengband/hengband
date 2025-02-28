#pragma once

class FloorType;
bool cave_clean_bold(const FloorType &floor, int y, int x);
bool cave_drop_bold(const FloorType &floor, int y, int x);
bool pattern_tile(const FloorType &floor, int y, int x);
