#pragma once

#include "system/angband.h"

struct object_type;;
class player_type;
void describe_flavor(player_type *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode);
