#pragma once

enum class StoreSaleType;
class PlayerType;
void store_prt_gold(int num_golds);
void display_entry(PlayerType *player_ptr, int pos, StoreSaleType store_num);
void display_store_inventory(PlayerType *player_ptr, StoreSaleType store_num);
void display_store(PlayerType *player_ptr, StoreSaleType store_num);
