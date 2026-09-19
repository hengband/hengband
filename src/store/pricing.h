#pragma once

#define LOW_PRICE_THRESHOLD 10L

class PlayerType;
class Store;
int price_item(PlayerType *player_ptr, int price, const Store &store, bool flip);
