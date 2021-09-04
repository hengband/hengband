#pragma once

#include "system/angband.h"
#include "system/game-option-types.h"

struct player_type;
void extract_option_vars(void);
void do_cmd_options_aux(player_type *player_ptr, game_option_types page, concptr info);
void do_cmd_options(player_type *player_ptr);
