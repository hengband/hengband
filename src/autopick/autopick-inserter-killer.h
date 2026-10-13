#pragma once

class AutopickTextBody;
void check_expression_line(AutopickTextBody *tb, int y);
bool can_insert_line(AutopickTextBody *tb, int add_num = 1);
bool insert_return_code(AutopickTextBody *tb);
bool insert_macro_line(AutopickTextBody *tb);
bool insert_keymap_line(AutopickTextBody *tb);
void insert_single_letter(AutopickTextBody *tb, int key);
void kill_line_segment(AutopickTextBody *tb, int y, int x0, int x1, bool whole);
