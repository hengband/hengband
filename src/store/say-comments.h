#pragma once

#include "system/angband.h"

enum class StoreSaleType;
class PlayerType;
void store_owner_says_comment(int price, StoreSaleType store_num);
void purchase_analyze(PlayerType *player_ptr, PRICE price, PRICE value, PRICE guess);
