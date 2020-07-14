#pragma once

#include "object-enchant/activation-info-table.h"
#include "system/angband.h"

int activation_index(player_type *player_ptr, object_type *o_ptr);
const activation_type *find_activation_info(player_type *player_ptr, object_type *o_ptr);
