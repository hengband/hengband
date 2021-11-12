#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
object_type *choose_cursed_obj_name(PlayerType *player_ptr, BIT_FLAGS flag);
void execute_cursed_items_effect(PlayerType *player_ptr);
