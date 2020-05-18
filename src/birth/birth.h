#pragma once

extern void add_history_from_pref_line(concptr t);
extern void player_birth(player_type *creature_ptr, void(*process_autopick_file_command)(char*));
extern void get_height_weight(player_type *creature_ptr);
extern void player_outfit(player_type *creature_ptr);
extern void dump_yourself(player_type *creature_ptr, FILE *fff);

