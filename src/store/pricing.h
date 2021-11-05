#pragma once

#include "system/angband.h"

#define LOW_PRICE_THRESHOLD 10L

struct object_type;
class PlayerType;
PRICE price_item(PlayerType *player_ptr, object_type *o_ptr, int greed, bool flip);
