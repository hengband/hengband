#pragma once

#include <string>
#include <string_view>

class PlayerType;
void process_player_name(PlayerType *player_ptr, bool is_new_savefile = false);
void get_name(PlayerType *player_ptr);
std::string make_player_base_name(std::string_view name);
std::string make_player_name_for_expression(std::string_view name);
