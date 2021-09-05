#pragma once

#include "system/angband.h"

struct player_type;
void wild_magic(player_type *caster_ptr, int spell);
void call_chaos(player_type *caster_ptr);
bool activate_ty_curse(player_type *target_ptr, bool stop_ty, int *count);
void cast_wonder(player_type *caster_ptr, DIRECTION dir);
