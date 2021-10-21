#pragma once

struct object_type;
struct player_type;
bool activate_dragon_breath(player_type *player_ptr, object_type *o_ptr);
bool activate_breath_fire(player_type *player_ptr, object_type *o_ptr);
bool activate_breath_cold(player_type *player_ptr, object_type *o_ptr);
