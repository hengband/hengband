#pragma once

struct object_type;;
struct player_type;
bool item_tester_hook_eatable(player_type *player_ptr, const object_type *o_ptr);
bool item_tester_hook_quaff(player_type *player_ptr, const object_type *o_ptr);
bool can_player_destroy_object(player_type *player_ptr, object_type *o_ptr);
