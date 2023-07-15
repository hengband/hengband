#pragma once

#include "system/angband.h"

struct text_body_type;
void toggle_keyword(text_body_type *tb, BIT_FLAGS flg);
void toggle_command_letter(text_body_type *tb, byte flg);
void add_keyword(text_body_type *tb, BIT_FLAGS flg);
bool add_empty_line(text_body_type *tb);
void kill_yank_chain(text_body_type *tb);
void add_str_to_yank(text_body_type *tb, concptr str);
void copy_text_to_yank(text_body_type *tb);
