#pragma once

#include "object/object-broken.h"

/*
 * This seems like a pretty standard "typedef"
 */
struct object_type;
class PlayerType;

void inventory_damage(PlayerType *player_ptr, const ObjectBreaker& breaker, int perc);
