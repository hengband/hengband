#pragma once

#include "system/angband.h"

struct player_type;
void chest_death(player_type *owner_ptr, bool scatter, POSITION y, POSITION x, OBJECT_IDX o_idx);
void chest_trap(player_type *target_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx);
