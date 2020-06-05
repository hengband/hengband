#pragma once

#include "system/angband.h"

void call_chaos(player_type *caster_ptr);
bool activate_ty_curse(player_type *target_ptr, bool stop_ty, int *count);
void ring_of_power(player_type *caster_ptr, DIRECTION dir);
