#pragma once

#include "system/angband.h"

struct dun_data_type;
class FloorType;
class PlayerType;
void add_river(FloorType *floor_ptr, dun_data_type *dd_ptr);
void build_streamer(PlayerType *player_ptr, FEAT_IDX feat, int chance);
void place_trees(POSITION x, POSITION y);
void destroy_level(PlayerType *player_ptr);
