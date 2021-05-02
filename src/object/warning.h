#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
object_type *choose_warning_item(player_type *creature_ptr);
bool process_warning(player_type *creature_ptr, POSITION xx, POSITION yy);
