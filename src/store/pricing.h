#pragma once

#define LOW_PRICE_THRESHOLD 10L

enum class StoreSaleType;
class PlayerType;
int price_item(PlayerType *player_ptr, int price, int greed, bool flip, StoreSaleType store_num);
