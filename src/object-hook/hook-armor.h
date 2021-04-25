#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool item_tester_hook_wear(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_cursed(player_type *player_ptr, object_type *o_ptr);
bool object_is_armour(player_type *player_ptr, object_type *o_ptr);
