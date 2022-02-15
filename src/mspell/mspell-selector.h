#pragma once

#include "system/angband.h"

enum class MonsterAbilityType;
struct msa_type;
class PlayerType;
MonsterAbilityType choose_attack_spell(PlayerType *player_ptr, msa_type *msa_ptr);
