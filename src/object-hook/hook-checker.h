#pragma once

typedef struct object_type object_type;
bool object_is_valid(const object_type *o_ptr);
bool object_is_held_monster(const object_type *o_ptr);
bool object_is_broken(const object_type *o_ptr);
bool object_is_cursed(const object_type *o_ptr);
