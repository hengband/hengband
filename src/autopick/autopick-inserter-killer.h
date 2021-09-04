#pragma once

struct text_body_type;
void check_expression_line(text_body_type *tb, int y);
bool can_insert_line(text_body_type *tb, int add_num = 1);
bool insert_return_code(text_body_type *tb);
bool insert_macro_line(text_body_type *tb);
bool insert_keymap_line(text_body_type *tb);
void insert_single_letter(text_body_type *tb, int key);
void kill_line_segment(text_body_type *tb, int y, int x0, int x1, bool whole);
