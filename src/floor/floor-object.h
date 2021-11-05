﻿#pragma once

#include "object/item-tester-hooker.h"
#include "system/angband.h"
#include <optional>

class ObjectIndexList;

struct floor_type;
struct object_type;
class PlayerType;
class ItemTester;
bool make_object(PlayerType *player_ptr, object_type *j_ptr, BIT_FLAGS mode, std::optional<int> rq_mon_level = std::nullopt);
bool make_gold(PlayerType *player_ptr, object_type *j_ptr);
void delete_all_items_from_floor(PlayerType *player_ptr, POSITION y, POSITION x);
void floor_item_increase(PlayerType *player_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void floor_item_optimize(PlayerType *player_ptr, INVENTORY_IDX item);
void delete_object_idx(PlayerType *player_ptr, OBJECT_IDX o_idx);
void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
ObjectIndexList &get_o_idx_list_contains(floor_type *floor_ptr, OBJECT_IDX o_idx);
OBJECT_IDX drop_near(PlayerType *player_ptr, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
void floor_item_charges(floor_type *player_ptr, INVENTORY_IDX item);
void floor_item_describe(PlayerType *player_ptr, INVENTORY_IDX item);
object_type *choose_object(PlayerType *player_ptr, OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, const ItemTester& item_tester = AllMatchItemTester());
