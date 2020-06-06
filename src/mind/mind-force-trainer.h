#pragma once

#include "system/angband.h"

MAGIC_NUM1 get_current_ki(player_type *caster_ptr);
void set_current_ki(player_type *caster_ptr, bool is_reset, MAGIC_NUM1 ki);
bool clear_mind(player_type *creature_ptr);
