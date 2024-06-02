#pragma once

#include <optional>
#include <string>

extern std::optional<std::string> histpref_buf;

class PlayerType;
int interpret_pref_file(PlayerType *player_ptr, char *buf);
