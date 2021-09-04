#pragma once

struct player_attack_type;
struct player_type;
void process_vorpal_attack(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
