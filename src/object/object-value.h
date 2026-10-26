#pragma once

#include "system/angband.h"

class ItemEntity;
class PlayerType;
PRICE clamp_price(int64_t value);
PRICE object_value_real(const ItemEntity *o_ptr);
