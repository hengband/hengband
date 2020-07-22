#pragma once

#include "system/angband.h"

void fetch_item(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
bool fetch_monster(player_type *caster_ptr);
