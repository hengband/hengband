#pragma once

#include "system/angband.h"
#include "combat/combat-options-type.h"

class PlayerType;
void exe_player_attack_to_monster(PlayerType *player_ptr, POSITION y, POSITION x, bool *fear, bool *mdeath, int16_t hand, combat_options mode);
void massacre(PlayerType *player_ptr);
