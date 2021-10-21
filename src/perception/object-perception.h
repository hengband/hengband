#pragma once

struct object_type;;
class player_type;
void object_known(object_type *o_ptr);
void object_aware(player_type *player_ptr, object_type *o_ptr);
void object_tried(object_type *o_ptr);
