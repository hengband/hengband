#pragma once

#include "system/angband.h"

object_type *choose_warning_item(player_type *creature_ptr);
bool process_warning(player_type *creature_ptr, POSITION xx, POSITION yy);
