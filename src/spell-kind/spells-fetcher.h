#pragma once

#include "system/angband.h"

struct player_type;
void fetch_item(player_type *player_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
bool fetch_monster(player_type *player_ptr);
