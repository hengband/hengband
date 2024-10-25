#pragma once

#include "system/angband.h"

enum game_option_page : int;
class PlayerType;
void extract_option_vars();
void do_cmd_options_aux(PlayerType *player_ptr, game_option_page page, concptr info);
void do_cmd_options(PlayerType *player_ptr);
