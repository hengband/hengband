#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class ObjectType;
class PlayerType;
bool is_active_torch(ObjectType *o_ptr);
void torch_flags(ObjectType *o_ptr, TrFlags &flgs);
void torch_dice(ObjectType *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
void torch_lost_fuel(ObjectType *o_ptr);
void update_lite_radius(PlayerType *player_ptr);
void update_lite(PlayerType *player_ptr);
