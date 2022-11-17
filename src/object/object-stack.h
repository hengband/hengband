#pragma once

class ItemEntity;
void distribute_charges(ItemEntity *o_ptr, ItemEntity *q_ptr, int amt);
void reduce_charges(ItemEntity *o_ptr, int amt);
int object_similar_part(const ItemEntity *o_ptr, const ItemEntity *j_ptr);
bool object_similar(const ItemEntity *o_ptr, const ItemEntity *j_ptr);
void object_absorb(ItemEntity *o_ptr, ItemEntity *j_ptr);
