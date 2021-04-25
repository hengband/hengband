#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool item_tester_hook_nameless_weapon_armour(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_identify(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_identify_weapon_armour(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_identify_fully(player_type *player_ptr, object_type *o_ptr);
bool item_tester_hook_identify_fully_weapon_armour(player_type *player_ptr, object_type *o_ptr);
