#pragma once

typedef struct player_attack_type player_attack_type;
typedef struct player_type player_type;
void process_vorpal_attack(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
