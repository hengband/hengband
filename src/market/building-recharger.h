#pragma once

#include <cstdint>

class ItemEntity;
class PlayerType;
int64_t calc_recharge_all_price(const ItemEntity &item);
void building_recharge(PlayerType *player_ptr);
void building_recharge_all(PlayerType *player_ptr);
