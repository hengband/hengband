#pragma once

#include <memory>
#include <span>
#include <tl/optional.hpp>

class BaseitemKey;
class ItemEntity;
class PlayerType;
char index_to_label(int i);
short wield_slot(PlayerType *player_ptr, const ItemEntity &item);
bool check_book_realm(PlayerType *player_ptr, const BaseitemKey &bi_key);
std::shared_ptr<ItemEntity> ref_item(PlayerType *player_ptr, short i_idx);
tl::optional<short> find_item_index(std::span<const std::shared_ptr<ItemEntity>> items, const std::shared_ptr<ItemEntity> &item);
tl::optional<short> find_current_i_idx(PlayerType *player_ptr, bool is_inventory, const std::shared_ptr<ItemEntity> &item);
