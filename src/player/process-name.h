#pragma once

typedef struct player_type player_type;
void process_player_name(player_type *creature_ptr, bool is_new_savefile = false);
void get_name(player_type *creature_ptr);
