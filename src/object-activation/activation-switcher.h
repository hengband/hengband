#pragma once

#include "system/angband.h"

struct activation_type;
struct object_type;
class PlayerType;
bool switch_activation(PlayerType *player_ptr, object_type **o_ptr_ptr, const activation_type *const act_ptr, concptr name);
