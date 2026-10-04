#pragma once

#include <memory>

class ItemEntity;
class PlayerType;
void inven_item_charges(const ItemEntity &item);
void describe_item_charges(PlayerType *player_ptr, bool is_inventory, const std::shared_ptr<ItemEntity> &item);
void inven_item_describe(PlayerType *player_ptr, short i_idx);
void display_koff(PlayerType *player_ptr);
