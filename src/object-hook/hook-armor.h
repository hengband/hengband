#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool item_tester_hook_wear(player_type *player_ptr, const object_type *o_ptr);
bool object_is_armour(const object_type *o_ptr);
