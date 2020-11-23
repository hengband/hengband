#pragma once

#include "system/angband.h"

void object_wipe(object_type *o_ptr);
void object_copy(object_type *o_ptr, object_type *j_ptr);
void object_prep(player_type *player_ptr, object_type *o_ptr, KIND_OBJECT_IDX k_idx);
