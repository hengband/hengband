#pragma once

struct object_type;;
struct player_type;
void wield_all(player_type *creature_ptr);
void add_outfit(player_type *creature_ptr, object_type *o_ptr);
void player_outfit(player_type *creature_ptr);
