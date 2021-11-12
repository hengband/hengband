#pragma once

#include "system/angband.h"

struct dun_data_type;
struct floor_type;
class PlayerType;
void add_river(floor_type *floor_ptr, dun_data_type *dd_ptr);
void build_streamer(PlayerType *player_ptr, FEAT_IDX feat, int chance);
void place_trees(PlayerType *player_ptr, POSITION x, POSITION y);
void destroy_level(PlayerType *player_ptr);
