#pragma once

enum class StoreSaleType;
class ObjectType;
class PlayerType;
bool store_will_buy(PlayerType *, const ObjectType *o_ptr, StoreSaleType store_num);
void mass_produce(PlayerType *, ObjectType *o_ptr, StoreSaleType store_num);
