#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
object_type *choose_warning_item(PlayerType *player_ptr);
bool process_warning(PlayerType *player_ptr, POSITION xx, POSITION yy);
