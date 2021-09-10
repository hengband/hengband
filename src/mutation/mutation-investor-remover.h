#pragma once

#include "system/angband.h"

struct player_type;
bool gain_mutation(player_type *player_ptr, MUTATION_IDX choose_mut);
bool lose_mutation(player_type *player_ptr, MUTATION_IDX choose_mut);
void lose_all_mutations(player_type *player_ptr);
