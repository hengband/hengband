#pragma once

#include <stdint.h>
#include <string_view>

class PlayerType;
bool show_file(PlayerType *player_ptr, bool show_version, std::string_view name_with_tag, int initial_line, uint32_t mode, std::string_view what = "");
void str_tolower(char *str);
