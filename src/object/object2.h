#pragma once

#include "system/angband.h"
#include "floor/floor.h"

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

s32b flag_cost(object_type *o_ptr, int plusses);
byte value_check_aux1(object_type *o_ptr);
byte value_check_aux2(object_type *o_ptr);
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
void reduce_charges(object_type *o_ptr, int amt);
int object_similar_part(object_type *o_ptr, object_type *j_ptr);
bool object_similar(object_type *o_ptr, object_type *j_ptr);
void object_absorb(object_type *o_ptr, object_type *j_ptr);
void apply_magic(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);
bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
