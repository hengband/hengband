#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
void apply_magic_to_object(PlayerType *player_ptr, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
