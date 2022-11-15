#pragma once

#include "system/angband.h"

struct activation_type;
class ItemEntity;
class PlayerType;
bool switch_activation(PlayerType *player_ptr, ItemEntity **o_ptr_ptr, const activation_type *const act_ptr, concptr name);
