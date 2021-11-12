#pragma once

#include "system/angband.h"

#include <vector>

class PlayerType;
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes);
std::vector<concptr> read_pickpref_text_lines(PlayerType *player_ptr, int *filename_mode_p);
bool write_text_lines(concptr filename, concptr *lines_list);
concptr pickpref_filename(PlayerType *player_ptr, int filename_mode);
