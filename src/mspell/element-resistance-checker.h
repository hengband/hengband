#pragma once

#include "system/angband.h"

typedef struct msr_type msr_type;
void add_cheat_remove_flags_element(player_type *target_ptr, msr_type *msr_ptr);
void check_element_resistance(msr_type *msr_ptr);
