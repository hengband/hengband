#pragma once

#include "system/angband.h"
#include "floor/floor.h"

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

s32b flag_cost(object_type *o_ptr, int plusses);
void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
void delete_object_idx(player_type *owner_ptr, OBJECT_IDX o_idx);

OBJECT_IDX o_pop(floor_type *floor_ptr);
OBJECT_IDX get_obj_num(player_type *o_ptr, DEPTH level, BIT_FLAGS mode);

byte value_check_aux1(object_type *o_ptr);
byte value_check_aux2(object_type *o_ptr);

void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
void reduce_charges(object_type *o_ptr, int amt);
int object_similar_part(object_type *o_ptr, object_type *j_ptr);
bool object_similar(object_type *o_ptr, object_type *j_ptr);
void object_absorb(object_type *o_ptr, object_type *j_ptr);
IDX lookup_kind(tval_type tval, OBJECT_SUBTYPE_VALUE sval);
void object_wipe(object_type *o_ptr);
void object_copy(object_type *o_ptr, object_type *j_ptr);
void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);

void apply_magic(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);

OBJECT_IDX drop_near(player_type *owner_type, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
void floor_item_charges(floor_type *owner_ptr, INVENTORY_IDX item);
bool inven_carry_okay(object_type *o_ptr);
bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
INVENTORY_IDX inven_takeoff(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
void floor_item_describe(player_type *player_ptr, INVENTORY_IDX item); // 暫定、元々object2.c の内部からのみ呼ばれていた.
