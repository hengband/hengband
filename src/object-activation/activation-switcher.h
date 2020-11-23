#pragma once

#include "system/angband.h"

typedef struct activation_type activation_type;
bool switch_activation(player_type *user_ptr, object_type *o_ptr, const activation_type *const act_ptr, concptr name);
