#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
object_type *choose_cursed_obj_name(player_type *creature_ptr, BIT_FLAGS flag);
void execute_cursed_items_effect(player_type *creature_ptr);
