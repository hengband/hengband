#pragma once

struct player_type;
void arena_comm(player_type *player_ptr, int cmd);
void update_gambling_monsters(player_type *player_ptr);
bool monster_arena_comm(player_type *player_ptr);
