#pragma once

#include <string>
#include <string_view>

class PlayerType;
int history_char_start_column(std::string_view line, int column);
std::string_view history_char_at_column(std::string_view line, int column);
void overwrite_history_char(std::string &line, int column, std::string_view ch);
void edit_history(PlayerType *player_ptr);
