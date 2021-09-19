#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
object_type *choose_warning_item(player_type *player_ptr);
bool process_warning(player_type *player_ptr, POSITION xx, POSITION yy);
