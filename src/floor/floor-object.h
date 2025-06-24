#pragma once

#include "object/item-tester-hooker.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-allocation.h"
#include "util/point-2d.h"
#include <tl/optional.hpp>
#include <vector>

class ObjectIndexList;

class FloorType;
class ItemEntity;
class PlayerType;
class ItemTester;
tl::optional<ItemEntity> make_object(PlayerType *player_ptr, BIT_FLAGS mode, BaseitemRestrict restrict = nullptr, tl::optional<int> rq_mon_level = tl::nullopt);
void delete_all_items_from_floor(PlayerType *player_ptr, const Pos2D &pos);
void floor_item_increase(PlayerType *player_ptr, INVENTORY_IDX i_idx, ITEM_NUMBER num);
void floor_item_optimize(PlayerType *player_ptr, INVENTORY_IDX i_idx);
void delete_object_idx(PlayerType *player_ptr, OBJECT_IDX o_idx);
void excise_object_idx(FloorType &floor, OBJECT_IDX o_idx);
void delete_items(PlayerType *player_ptr, std::vector<OBJECT_IDX> delete_i_idx_list);
void delete_items(PlayerType *player_ptr, ObjectIndexList &o_idx_list);
ObjectIndexList &get_o_idx_list_contains(FloorType &floor, OBJECT_IDX o_idx);
short drop_near(PlayerType *player_ptr, ItemEntity &drop_item, const Pos2D &pos, tl::optional<int> chance = tl::nullopt, bool show_drop_message = true);
void floor_item_charges(const FloorType &floor, INVENTORY_IDX i_idx);
void floor_item_describe(PlayerType *player_ptr, INVENTORY_IDX i_idx);
ItemEntity *choose_object(PlayerType *player_ptr, short *initial_i_idx, concptr q, concptr s, BIT_FLAGS option, const ItemTester &item_tester = AllMatchItemTester());
