#pragma once

#include "system/angband.h"

class DungeonData;
class FloorType;
class PlayerType;
void add_river(FloorType *floor_ptr, DungeonData *dd_ptr);
void build_streamer(PlayerType *player_ptr, FEAT_IDX feat, int chance);
void place_trees(PlayerType *player_ptr, POSITION x, POSITION y);
void destroy_level(PlayerType *player_ptr);
