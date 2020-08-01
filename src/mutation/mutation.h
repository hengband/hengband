#pragma once

#include "system/angband.h"

int calc_mutant_regenerate_mod(player_type *creature_ptr);
bool exe_mutation_power(player_type *creature_ptr, int power);
void become_living_trump(player_type *creature_ptr);
void set_mutation_flags(player_type *creature_ptr);
