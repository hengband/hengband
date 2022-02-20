#pragma once

#include "system/angband.h"

#define LOW_PRICE_THRESHOLD 10L

class ObjectType;
class PlayerType;
PRICE price_item(PlayerType *player_ptr, ObjectType *o_ptr, int greed, bool flip);
