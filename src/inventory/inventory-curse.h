#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
object_type *choose_cursed_obj_name(player_type *player_ptr, BIT_FLAGS flag);
void execute_cursed_items_effect(player_type *player_ptr);
