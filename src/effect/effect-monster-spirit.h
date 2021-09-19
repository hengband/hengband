#pragma once

#include "system/angband.h"

struct effect_monster_type;
struct player_type;
process_result effect_monster_drain_mana(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_mind_blast(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_brain_smash(player_type *player_ptr, effect_monster_type *em_ptr);
