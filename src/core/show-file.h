#pragma once

#include "system/angband.h"

class PlayerType;
bool show_file(PlayerType *player_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode);
void str_tolower(char *str);
