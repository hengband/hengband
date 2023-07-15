#pragma once

struct player_attack_type;
class PlayerType;
void process_vorpal_attack(PlayerType *player_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
