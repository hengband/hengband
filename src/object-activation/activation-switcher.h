#pragma once

#include "system/angband.h"

struct activation_type;
struct object_type;;
struct player_type;
bool switch_activation(player_type *player_ptr, object_type **o_ptr_ptr, const activation_type *const act_ptr, concptr name);
