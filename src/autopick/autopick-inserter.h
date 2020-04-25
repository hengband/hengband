#pragma once

void check_expression_line(text_body_type *tb, int y);
bool insert_return_code(text_body_type *tb);
bool insert_macro_line(text_body_type *tb);
bool insert_keymap_line(text_body_type *tb);
void insert_single_letter(text_body_type *tb, int key);
