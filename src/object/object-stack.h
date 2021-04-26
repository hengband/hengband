#pragma once

typedef struct object_type object_type;
void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
void reduce_charges(object_type *o_ptr, int amt);
int object_similar_part(object_type *o_ptr, object_type *j_ptr);
bool object_similar(object_type *o_ptr, object_type *j_ptr);
void object_absorb(object_type *o_ptr, object_type *j_ptr);
