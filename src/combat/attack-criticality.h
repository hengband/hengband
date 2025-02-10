#pragma once

#include "combat/combat-options-type.h"
#include "main/sound-definitions-table.h"
#include "system/angband.h"
#include <string>
#include <tuple>

struct player_attack_type;
class PlayerType;
std::tuple<int, std::string, SoundKind> apply_critical_norm_damage(int k, int base_dam, int mult = 1);
int critical_norm(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, combat_options mode, bool impact = false);
void critical_attack(PlayerType *player_ptr, player_attack_type *pa_ptr);
