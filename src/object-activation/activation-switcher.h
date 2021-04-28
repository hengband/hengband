#pragma once

#include "system/angband.h"

typedef struct activation_type activation_type;
typedef struct object_type object_type;
typedef struct player_type player_type;
bool switch_activation(player_type *user_ptr, object_type **o_ptr_ptr, const activation_type *const act_ptr, concptr name);
