#pragma once

#include "system/angband.h"

bool do_cmd_cast_learned(player_type *caster_ptr);
void learn_spell(player_type *learner_ptr, int monspell);
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode);
