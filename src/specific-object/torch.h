#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class ItemEntity;
class PlayerType;
bool is_active_torch(ItemEntity *o_ptr);
void torch_flags(ItemEntity *o_ptr, TrFlags &flags);
void torch_dice(ItemEntity *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
void torch_lost_fuel(ItemEntity *o_ptr);
void update_lite_radius(PlayerType *player_ptr);
void update_lite(PlayerType *player_ptr);
