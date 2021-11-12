#pragma once

#include "system/angband.h"

enum class MonsterAbilityType;
typedef struct msa_type msa_type;
struct player_type;
MonsterAbilityType choose_attack_spell(player_type *player_ptr, msa_type *msa_ptr);
