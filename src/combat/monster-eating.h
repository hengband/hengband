#pragma once

#include "system/angband.h"
#include "combat/monster-attack-util.h"

void process_eat_gold(player_type *target_ptr, monap_type *monap_ptr);
bool check_eat_item(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_item(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_food(player_type *target_ptr, monap_type *monap_ptr);
void process_eat_lite(player_type *target_ptr, monap_type *monap_ptr);
