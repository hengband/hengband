#pragma once

#include "system/angband.h"
#include <string_view>

enum class AutopickMethod;
class AutopickTextBody;
void toggle_keyword(AutopickTextBody *tb, BIT_FLAGS flg);
void toggle_command_letter(AutopickTextBody *tb, AutopickMethod am);
void add_keyword(AutopickTextBody *tb, BIT_FLAGS flg);
bool add_empty_line(AutopickTextBody *tb);
void kill_yank_chain(AutopickTextBody *tb);
void add_str_to_yank(AutopickTextBody *tb, std::string_view str);
void copy_text_to_yank(AutopickTextBody *tb);
bool is_second_byte_of_kanji(std::string_view line, int pos);
