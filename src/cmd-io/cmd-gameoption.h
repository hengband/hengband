#pragma once

#include "system/angband.h"
#include "system/game-option-types.h"

class PlayerType;
void extract_option_vars(void);
void do_cmd_options_aux(PlayerType *player_ptr, game_option_types page, concptr info);
void do_cmd_options(PlayerType *player_ptr);
