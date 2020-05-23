#pragma once

#include "combat/player-attack-util.h"

void process_surprise_attack(player_type *attacker_ptr, player_attack_type *pa_ptr);
void print_surprise_attack(player_attack_type *pa_ptr);
void calc_surprise_attack_damage(player_type *attacker_ptr, player_attack_type *pa_ptr);
