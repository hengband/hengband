#pragma once

class ItemEntity;
class PlayerType;
void inven_item_charges(const ItemEntity &item);
void inven_item_describe(PlayerType *player_ptr, short item);
void display_koff(PlayerType *player_ptr, short bi_id);
