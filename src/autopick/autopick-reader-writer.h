#pragma once

concptr *read_pickpref_text_lines(player_type *player_ptr, int *filename_mode_p);
bool write_text_lines(concptr filename, concptr *lines_list);
concptr pickpref_filename(player_type *player_ptr, int filename_mode);
void add_autopick_list(autopick_type *entry);
