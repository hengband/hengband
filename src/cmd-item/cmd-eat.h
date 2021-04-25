#pragma once

#include "system/angband.h"

typedef struct player_type player_type;
void do_cmd_eat_food(player_type *creature_ptr);
void exe_eat_food(player_type *creature_ptr, INVENTORY_IDX item);
