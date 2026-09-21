#pragma once

class PlayerType;
class Store;
void store_prt_gold(int num_golds);
void display_entry(PlayerType *player_ptr, const Store &store, int pos);
void display_store_inventory(PlayerType *player_ptr, const Store &store);
void display_store(PlayerType *player_ptr, const Store &store);
