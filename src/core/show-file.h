#pragma once

#include <cstdint>
#include <string_view>

bool show_file(std::string_view player_name, bool show_version, std::string_view name_with_tag, int initial_line, uint32_t mode, std::string_view what = "");
void str_tolower(char *str);
