#pragma once

#include "system/angband.h"
#include "floor/floor.h"
#include "object-enchant/item-feeling.h"

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

s32b flag_cost(object_type *o_ptr, int plusses);
item_feel_type pseudo_value_check_heavy(object_type *o_ptr);
item_feel_type pseudo_value_check_light(object_type *o_ptr);
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
void reduce_charges(object_type *o_ptr, int amt);
int object_similar_part(object_type *o_ptr, object_type *j_ptr);
bool object_similar(object_type *o_ptr, object_type *j_ptr);
void object_absorb(object_type *o_ptr, object_type *j_ptr);
bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
