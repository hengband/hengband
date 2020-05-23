#pragma once

#include "system/angband.h"
#include "combat/player-attack-util.h"

void decide_blood_sucking(player_type *attacker_ptr, player_attack_type *pa_ptr);
void calc_drain(player_attack_type *pa_ptr);
void process_drain(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool is_human, bool *drain_msg);
