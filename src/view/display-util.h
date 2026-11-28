#pragma once

#include "system/angband.h"
#include <string>
#include <string_view>

std::string format_item_weight(int weight, bool with_unit = true);
void display_player_one_line(int entry, std::string_view val, TERM_COLOR attr);
int display_wrap_around(std::string_view sv, size_t width, int start_row, int col);
