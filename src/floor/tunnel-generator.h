#pragma once

#include "util/point-2d.h"

class DungeonData;
struct dt_type;
class PlayerType;
bool build_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, dt_type *dt_ptr, const Pos2D &pos_start, const Pos2D &pos_end);
bool build_tunnel2(PlayerType *player_ptr, DungeonData *dd_ptr, const Pos2D &pos_start, const Pos2D &pos_end, int type, int cutoff);
