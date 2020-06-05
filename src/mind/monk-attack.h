#pragma once

#include "system/angband.h"
#include "combat/player-attack-util.h"

void process_monk_attack(player_type *attacker_ptr, player_attack_type *pa_ptr);
bool double_attack(player_type *creature_ptr);
