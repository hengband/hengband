#pragma once

#include "system/angband.h"

struct player_type;
void chest_death(player_type *player_ptr, bool scatter, POSITION y, POSITION x, OBJECT_IDX o_idx);
void chest_trap(player_type *player_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx);
