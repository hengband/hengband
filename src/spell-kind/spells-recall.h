#pragma once

#include "system/angband.h"

bool recall_player(player_type *creature_ptr, TIME_EFFECT turns);
bool free_level_recall(player_type *creature_ptr);
bool reset_recall(player_type *caster_ptr);
