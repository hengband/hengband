#pragma once

struct player_type;
void do_cmd_save_screen_html_aux(char *filename, int message);
void do_cmd_save_screen(player_type *creature_ptr);
void do_cmd_load_screen(void);
