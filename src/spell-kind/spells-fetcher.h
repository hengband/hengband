#pragma once

#include "system/angband.h"

class PlayerType;
class Direction;
void fetch_item(PlayerType *player_ptr, const Direction &dir, WEIGHT wgt, bool require_los);
bool fetch_monster(PlayerType *player_ptr);
