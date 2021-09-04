#pragma once

#include "system/angband.h"

struct object_type;;
struct player_type;
void apply_magic_weapon(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power);
