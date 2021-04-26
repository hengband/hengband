#pragma once

typedef struct player_type player_type;
void process_player_hp_mp(player_type *creature_ptr);
bool hp_player(player_type *creature_ptr, int num);
