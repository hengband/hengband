#pragma once

#include "system/angband.h"

struct player_type;
void store_owner_says_comment(player_type *player_ptr);
void purchase_analyze(player_type *player_ptr, PRICE price, PRICE value, PRICE guess);
