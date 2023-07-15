#pragma once

#include "system/angband.h"

class PlayerType;
bool gain_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut);
bool lose_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut);
void lose_all_mutations(PlayerType *player_ptr);
