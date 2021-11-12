#pragma once

#include "system/angband.h"

class PlayerType;
void fetch_item(PlayerType *player_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
bool fetch_monster(PlayerType *player_ptr);
