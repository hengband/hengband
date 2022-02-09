#pragma once

class ObjectType;
void distribute_charges(ObjectType *o_ptr, ObjectType *q_ptr, int amt);
void reduce_charges(ObjectType *o_ptr, int amt);
int object_similar_part(const ObjectType *o_ptr, const ObjectType *j_ptr);
bool object_similar(const ObjectType *o_ptr, const ObjectType *j_ptr);
void object_absorb(ObjectType *o_ptr, ObjectType *j_ptr);
