#pragma once

#include "system/angband.h"

enum class RF_ABILITY;
typedef struct msa_type msa_type;
class PlayerType;
RF_ABILITY choose_attack_spell(PlayerType *player_ptr, msa_type *msa_ptr);
