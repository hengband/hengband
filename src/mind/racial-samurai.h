#pragma once

#include "combat/player-attack-util.h"

void concentration(player_type* creature_ptr);
bool choose_kata(player_type* creature_ptr);
int calc_attack_quality(player_type *attacker_ptr, player_attack_type *pa_ptr);
