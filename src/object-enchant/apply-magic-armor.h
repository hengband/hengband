#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
void apply_magic_armor(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
