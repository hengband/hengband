#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

struct object_type;;
struct player_type;
void torch_flags(object_type *o_ptr, TrFlags &flgs);
void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
void torch_lost_fuel(object_type *o_ptr);
void update_lite_radius(player_type *creature_ptr);
void update_lite(player_type *subject_ptr);
