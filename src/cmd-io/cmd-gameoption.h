#pragma once

#include "system/angband.h"

void extract_option_vars(void);
void do_cmd_options_aux(player_type *player_ptr, int page, concptr info);
void do_cmd_options(player_type *player_ptr);
