#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool item_tester_hook_eatable(player_type *player_ptr, const object_type *o_ptr);
bool item_tester_hook_quaff(player_type *player_ptr, const object_type *o_ptr);
bool object_is_readable(const object_type *o_ptr);
bool object_is_refill_lantern(const object_type *o_ptr);
bool object_can_refill_torch(const object_type *o_ptr);
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr);
bool object_is_potion(const object_type *o_ptr);
