#pragma once

void do_cmd_save_screen_html_aux(char *filename, int message);
void do_cmd_save_screen(player_type *creature_ptr, void(*process_autopick_file_command)(char*));
void do_cmd_load_screen(void);
