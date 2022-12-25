#pragma once

#include "system/angband.h"
#include <string_view>

void display_player_one_line(int entry, std::string_view val, TERM_COLOR attr);
int display_wrap_around(std::string_view sv, size_t width, int start_row, int col);
