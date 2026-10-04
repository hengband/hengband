#pragma once

class ItemEntity;
class PlayerType;
class Store;
enum class StoreSaleType;
int home_carry(PlayerType *player_ptr, Store &store, ItemEntity *o_ptr);
bool combine_and_reorder_home(PlayerType *player_ptr, Store &store);
bool combine_and_reorder_home(PlayerType *player_ptr, const StoreSaleType store_num);
