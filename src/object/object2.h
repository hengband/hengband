#pragma once

#include "floor/floor.h"

extern OBJECT_SUBTYPE_VALUE coin_type;
extern s32b flag_cost(object_type *o_ptr, int plusses);

extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

int bow_tval_ammo(object_type *o_ptr);

void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
void delete_object_idx(player_type *owner_ptr, OBJECT_IDX o_idx);
void delete_object(player_type *owner_ptr, POSITION y, POSITION x);

OBJECT_IDX o_pop(floor_type *floor_ptr);
OBJECT_IDX get_obj_num(player_type *o_ptr, DEPTH level, BIT_FLAGS mode);
void object_known(object_type *o_ptr);
void object_aware(player_type *owner_ptr, object_type *o_ptr);
void object_tried(object_type *o_ptr);

byte value_check_aux1(object_type *o_ptr);
byte value_check_aux2(object_type *o_ptr);

PRICE object_value(object_type *o_ptr);
PRICE object_value_real(object_type *o_ptr);
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

bool make_object(player_type *owner_ptr, object_type *j_ptr, BIT_FLAGS mode);
bool make_gold(floor_type *floor_ptr, object_type *j_ptr);
OBJECT_IDX drop_near(player_type *owner_type, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_charges(player_type *owner_ptr, INVENTORY_IDX item);
void inven_item_describe(player_type *owner_ptr, INVENTORY_IDX item);
void inven_item_increase(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
void floor_item_charges(floor_type *owner_ptr, INVENTORY_IDX item);
void floor_item_increase(floor_type *floor_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
void floor_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
bool inven_carry_okay(object_type *o_ptr);
bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
s16b inven_carry(player_type *owner_ptr, object_type *o_ptr);
INVENTORY_IDX inven_takeoff(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
void drop_from_inventory(player_type *owner_type, INVENTORY_IDX item, ITEM_NUMBER amt);
void combine_pack(player_type *owner_ptr);
void reorder_pack(player_type *owner_ptr);
void display_koff(player_type *owner_ptr, KIND_OBJECT_IDX k_idx);
void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
void torch_lost_fuel(object_type *o_ptr);
