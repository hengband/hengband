#pragma once

struct object_type;;
class player_type;
void wield_all(player_type *player_ptr);
void add_outfit(player_type *player_ptr, object_type *o_ptr);
void player_outfit(player_type *player_ptr);
