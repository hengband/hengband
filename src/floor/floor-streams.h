#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class DungeonData;
class FloorType;
class PlayerType;
void add_river(FloorType &floor, DungeonData *dd_ptr);
void build_streamer(PlayerType *player_ptr, FEAT_IDX feat, int chance);
void place_trees(PlayerType *player_ptr, const Pos2D &pos);
void destroy_level(PlayerType *player_ptr);
