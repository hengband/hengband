#pragma once

#include "system/angband.h"

struct player_type;
void msg_print_wizard(player_type *player_ptr, int cheat_type, concptr msg);
void msg_format_wizard(player_type *player_ptr, int cheat_type, concptr fmt, ...);
