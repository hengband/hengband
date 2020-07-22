#pragma once

#include "system/angband.h"

PRICE price_item(player_type *player_ptr, object_type *o_ptr, int greed, bool flip);
bool noneedtobargain(PRICE minprice);
