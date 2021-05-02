#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void exe_read(player_type *creature_ptr, INVENTORY_IDX item, bool known);
