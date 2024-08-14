#pragma once

#include "combat/combat-options-type.h"
#include "main/sound-definitions-table.h"
#include "system/angband.h"

#include <tuple>

struct player_attack_type;
class PlayerType;
std::string make_critical_message(std::string_view msg, bool supercritical);
std::tuple<int, std::string, sound_type> apply_critical_norm_damage(int k, int base_dam, bool supercritical, int mult = 1);
int critical_norm(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, combat_options mode, bool supercritical = false, bool impact = false);
void critical_attack(PlayerType *player_ptr, player_attack_type *pa_ptr);
