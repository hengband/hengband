#pragma once

#include <string_view>

enum class GameOptionPage : int;
class PlayerType;
void extract_option_vars();
void do_cmd_options_aux(PlayerType *player_ptr, GameOptionPage page, std::string_view info);
void do_cmd_options(PlayerType *player_ptr);
