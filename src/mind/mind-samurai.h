#pragma once

#include "system/angband.h"
#include "combat/monster-attack-util.h"
#include "combat/player-attack-util.h"

void concentration(player_type* creature_ptr);
bool choose_kata(player_type* creature_ptr);
int calc_attack_quality(player_type *attacker_ptr, player_attack_type *pa_ptr);
void mineuchi(player_type *attacker_ptr, player_attack_type *pa_ptr);
void musou_counterattack(player_type *target_ptr, monap_type *monap_ptr);
