#pragma once

#include "system/angband.h"

bool cast_learned_spell(player_type *caster_ptr, int spell, bool success);
void learn_spell(player_type *learner_ptr, int monspell);
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, BIT_FLAGS mode);
