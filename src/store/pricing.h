#pragma once

#define LOW_PRICE_THRESHOLD 10L

enum class StoreSaleType;
class ItemEntity;
class PlayerType;
int price_item(PlayerType *player_ptr, const ItemEntity *o_ptr, int greed, bool flip, StoreSaleType store_num);
