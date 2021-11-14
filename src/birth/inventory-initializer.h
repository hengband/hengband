#pragma once

struct object_type;
class PlayerType;
void wield_all(PlayerType *player_ptr);
void add_outfit(PlayerType *player_ptr, object_type *o_ptr);
void player_outfit(PlayerType *player_ptr);
