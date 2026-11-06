#pragma once

class DisplaySymbolPair;
namespace original_tiles {
extern bool double_map_size;
int map_tile_scale();
int map_tile_columns();
int map_tile_rows();
bool queue_map_tile(int x, int y, DisplaySymbolPair symbols);
}
