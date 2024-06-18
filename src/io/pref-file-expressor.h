#pragma once

#include "system/angband.h"
#include <string>

class PlayerType;
std::string process_pref_file_expr(PlayerType *player_ptr, char **sp, char *fp);
