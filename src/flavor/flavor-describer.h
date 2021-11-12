#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
void describe_flavor(PlayerType *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode);
