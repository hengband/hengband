#pragma once

#include "system/angband.h"
#include "object/object-util.h"

void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
void torch_lost_fuel(object_type *o_ptr);
