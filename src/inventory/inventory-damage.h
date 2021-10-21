#pragma once

#include "object/object-broken.h"

/*
 * This seems like a pretty standard "typedef"
 */
struct object_type;;
class player_type;

void inventory_damage(player_type *player_ptr, const ObjectBreaker& breaker, int perc);
