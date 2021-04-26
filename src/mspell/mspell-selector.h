#pragma once

#include "system/angband.h"

enum class RF_ABILITY;
typedef struct msa_type msa_type;
typedef struct player_type player_type;
RF_ABILITY choose_attack_spell(player_type *target_ptr, msa_type *msa_ptr);
