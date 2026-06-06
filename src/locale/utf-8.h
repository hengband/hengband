#pragma once

#include <string_view>

int utf8_next_char_byte_length(std::string_view str);
bool is_utf8_str(std::string_view str);
