#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
void object_flags(player_type *player_ptr, object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
void object_flags_known(player_type *player_ptr, object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
