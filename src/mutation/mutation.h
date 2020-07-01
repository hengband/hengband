#pragma once

#include "system/angband.h"

bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut);
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut);
void lose_all_mutations(player_type *creature_ptr);
int calc_mutant_regenerate_mod(player_type *creature_ptr);
bool exe_mutation_power(player_type *creature_ptr, int power);
void become_living_trump(player_type *creature_ptr);
void set_mutation_flags(player_type *creature_ptr);
