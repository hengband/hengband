#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class DungeonData;
struct dt_type;
class PlayerType;
bool build_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, dt_type *dt_ptr, const Pos2D &pos_start_initial, const Pos2D &pos_end);
bool build_tunnel2(PlayerType *player_ptr, DungeonData *dd_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
