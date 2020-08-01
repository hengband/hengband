#pragma once

#include "system/angband.h"

bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut);
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut);
void lose_all_mutations(player_type *creature_ptr);
