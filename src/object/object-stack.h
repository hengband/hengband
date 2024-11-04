#pragma once

class ItemEntity;
void distribute_charges(ItemEntity *o_ptr, ItemEntity *q_ptr, int amt);
void reduce_charges(ItemEntity *o_ptr, int amt);
void object_absorb(ItemEntity *o_ptr, ItemEntity *j_ptr);
