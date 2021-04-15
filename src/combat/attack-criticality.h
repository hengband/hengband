#pragma once

#include "system/angband.h"
#include "player-attack/player-attack-util.h"

HIT_POINT critical_norm(player_type *attacker_ptr, WEIGHT weight, int plus, HIT_POINT dam, s16b meichuu, combat_options mode, bool impact = false);
int calc_monster_critical(DICE_NUMBER dice, DICE_SID sides, HIT_POINT dam);
void critical_attack(player_type *attacker_ptr, player_attack_type *pa_ptr);
