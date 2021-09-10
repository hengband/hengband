#pragma once

#include "system/angband.h"

struct player_type;
void wild_magic(player_type *player_ptr, int spell);
void call_chaos(player_type *player_ptr);
bool activate_ty_curse(player_type *player_ptr, bool stop_ty, int *count);
void cast_wonder(player_type *player_ptr, DIRECTION dir);
