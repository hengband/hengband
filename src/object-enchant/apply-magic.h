#pragma once

#include "system/angband.h"

struct object_type;
class player_type;
void apply_magic_to_object(player_type *player_ptr, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
