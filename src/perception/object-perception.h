#pragma once

struct object_type;
class PlayerType;
void object_known(object_type *o_ptr);
void object_aware(PlayerType *player_ptr, object_type *o_ptr);
void object_tried(object_type *o_ptr);
