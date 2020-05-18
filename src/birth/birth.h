#pragma once

// 暫定。後で移す.
extern s32b stat_match[6];
extern s32b auto_round;

extern void add_history_from_pref_line(concptr t);
extern void player_birth(player_type *creature_ptr, void(*process_autopick_file_command)(char*));
extern void player_outfit(player_type *creature_ptr);
