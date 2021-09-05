#pragma once

#include "object/object-broken.h"

/*
 * This seems like a pretty standard "typedef"
 */
struct object_type;;
struct player_type;

void inventory_damage(player_type *creature_ptr, const ObjectBreaker& breaker, int perc);
