#pragma once

#include "system/angband.h"

class DungeonData;
class PlayerType;
bool find_space(PlayerType *player_ptr, DungeonData *dd_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width);
