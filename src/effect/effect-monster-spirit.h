﻿#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
process_result effect_monster_drain_mana(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_mind_blast(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_brain_smash(PlayerType *player_ptr, effect_monster_type *em_ptr);
