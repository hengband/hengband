#pragma once

#include "system/angband.h"

class PlayerType;
void store_owner_says_comment(PlayerType *player_ptr);
void purchase_analyze(PlayerType *player_ptr, PRICE price, PRICE value, PRICE guess);
