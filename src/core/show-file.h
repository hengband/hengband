#pragma once

#include "system/angband.h"

struct player_type;
bool show_file(player_type *player_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode);
void str_tolower(char *str);
