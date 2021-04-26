#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
void object_known(object_type *o_ptr);
void object_aware(player_type *owner_ptr, object_type *o_ptr);
void object_tried(object_type *o_ptr);

bool object_is_aware(object_type *o_ptr);
bool object_is_tried(object_type *o_ptr);
bool object_is_known(object_type *o_ptr);
bool object_is_fully_known(object_type *o_ptr);
