#pragma once

#include "system/angband.h"

struct player_type;
void do_cmd_eat_food(player_type *player_ptr);
void exe_eat_food(player_type *player_ptr, INVENTORY_IDX item);
