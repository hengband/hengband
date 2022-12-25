#pragma once

enum class StoreSaleType;
class ItemEntity;
class PlayerType;
bool store_will_buy(PlayerType *, const ItemEntity *o_ptr, StoreSaleType store_num);
void mass_produce(ItemEntity *o_ptr, StoreSaleType store_num);
