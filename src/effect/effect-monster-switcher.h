#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
process_result switch_effects_monster(PlayerType *player_ptr, effect_monster_type *em_ptr);
