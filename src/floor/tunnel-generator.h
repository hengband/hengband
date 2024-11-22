#pragma once

#include "system/angband.h"

class DungeonData;
struct dt_type;
class PlayerType;
bool build_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, dt_type *dt_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2);
bool build_tunnel2(PlayerType *player_ptr, DungeonData *dd_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
