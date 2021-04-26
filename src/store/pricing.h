#pragma once

#include "system/angband.h"

#define LOW_PRICE_THRESHOLD 10L

typedef struct object_type object_type;
typedef struct player_type player_type;
PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip);
