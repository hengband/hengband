#pragma once

struct player_attack_type;
class player_type;
void process_vorpal_attack(player_type *player_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
