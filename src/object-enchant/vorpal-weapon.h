#pragma once

#include "system/angband.h"
#include "player-attack/player-attack-util.h"

void process_vorpal_attack(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool vorpal_cut, const int vorpal_chance);
