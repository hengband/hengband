#pragma once

#include "system/angband.h"

typedef enum blue_magic_type blue_magic_type;
void learn_spell(player_type *learner_ptr, int monspell);
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, blue_magic_type mode);
