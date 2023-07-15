#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
ItemEntity *choose_cursed_obj_name(PlayerType *player_ptr, BIT_FLAGS flag);
void execute_cursed_items_effect(PlayerType *player_ptr);
