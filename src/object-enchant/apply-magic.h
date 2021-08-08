#pragma once

#include "system/angband.h"

struct object_type;
struct player_type;
void apply_magic_to_object(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
