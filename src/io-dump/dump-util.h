#pragma once

#include "system/angband.h"
#include "system/terrain-type-definition.h"
#include <map>

#define FILE_NAME_SIZE 1024

class ColoredChar;
class ColoredCharsClipboard {
public:
    ColoredCharsClipboard(const ColoredCharsClipboard &) = delete;
    ColoredCharsClipboard(ColoredCharsClipboard &&) = delete;
    ColoredCharsClipboard &operator=(const ColoredCharsClipboard &) = delete;
    ColoredCharsClipboard &operator=(ColoredCharsClipboard &&) = delete;
    ~ColoredCharsClipboard() = default;

    static ColoredCharsClipboard &get_instance();

    ColoredChar cc;
    std::map<int, ColoredChar> cc_map;

private:
    ColoredCharsClipboard();
    static ColoredCharsClipboard instance;
};

bool visual_mode_command(char ch, bool *visual_list_ptr, int height, int width,
    TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
    TERM_COLOR *cur_attr_ptr, char *cur_char_ptr, bool *need_redraw);

bool open_temporary_file(FILE **fff, char *file_name);
void browser_cursor(char ch, int *column, IDX *grp_cur, int grp_cnt, IDX *list_cur, int list_cnt);
void display_group_list(int col, int row, int wid, int per_page, IDX grp_idx[], concptr group_text[], int grp_cur, int grp_top);
void display_visual_list(int col, int row, int height, int width, TERM_COLOR attr_top, byte char_left);
void place_visual_list_cursor(TERM_LEN col, TERM_LEN row, TERM_COLOR a, byte c, TERM_COLOR attr_top, byte char_left);
