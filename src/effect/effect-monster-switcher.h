#pragma once

#include "system/angband.h"

struct effect_monster_type;
class player_type;
process_result switch_effects_monster(player_type *player_ptr, effect_monster_type *em_ptr);
