#pragma once

#include "system/angband.h"

struct dun_data_type;
struct floor_type;
struct player_type;
void add_river(floor_type *floor_ptr, dun_data_type *dd_ptr);
void build_streamer(player_type *player_ptr, FEAT_IDX feat, int chance);
void place_trees(player_type *player_ptr, POSITION x, POSITION y);
void destroy_level(player_type *player_ptr);
