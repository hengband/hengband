#pragma once

#include <string>
#include <tl/optional.hpp>

extern tl::optional<std::string> histpref_buf;

class PlayerType;
int interpret_pref_file(PlayerType *player_ptr, char *buf);
