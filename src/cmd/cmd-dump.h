#pragma once

extern void do_cmd_pref(player_type *creature_ptr);
extern void do_cmd_colors(player_type *creature_ptr, void(*process_autopick_file_command)(char*));
extern void do_cmd_note(void);
extern void do_cmd_version(void);
extern void do_cmd_feeling(player_type *creature_ptr);
extern void do_cmd_time(player_type *creature_ptr);
