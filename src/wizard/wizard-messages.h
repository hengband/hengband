#pragma once

#include "system/angband.h"
#include <string_view>

class PlayerType;
void msg_print_wizard(PlayerType *player_ptr, int cheat_type, std::string_view msg);
void msg_format_wizard(PlayerType *player_ptr, int cheat_type, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
