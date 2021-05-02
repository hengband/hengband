#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
void describe_flavor(player_type *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode);
