#pragma once

class PlayerType;
class StoreScreen;
void store_prt_gold(const StoreScreen &screen, int num_golds);
void display_entry(PlayerType *player_ptr, const StoreScreen &screen, int pos);
void display_store_inventory(PlayerType *player_ptr, const StoreScreen &screen);
void display_store(PlayerType *player_ptr, const StoreScreen &screen);
