#pragma once

#include "system/angband.h"

struct player_type;
void autopick_load_pref(player_type *player_ptr, bool disp_mes);
concptr *read_pickpref_text_lines(player_type *player_ptr, int *filename_mode_p);
bool write_text_lines(concptr filename, concptr *lines_list);
concptr pickpref_filename(player_type *player_ptr, int filename_mode);
