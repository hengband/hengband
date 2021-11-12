#pragma once

#include "system/angband.h"

class PlayerType;
void msg_print_wizard(PlayerType *player_ptr, int cheat_type, concptr msg);
void msg_format_wizard(PlayerType *player_ptr, int cheat_type, concptr fmt, ...);
