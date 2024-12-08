#pragma once

#include "system/angband.h"
#include "view/display-symbol.h"
#include <map>
#include <string>
#include <vector>

#define FILE_NAME_SIZE 1024

class DisplaySymbol;
class DisplaySymbolsClipboard {
public:
    DisplaySymbolsClipboard(const DisplaySymbolsClipboard &) = delete;
    DisplaySymbolsClipboard(DisplaySymbolsClipboard &&) = delete;
    DisplaySymbolsClipboard &operator=(const DisplaySymbolsClipboard &) = delete;
    DisplaySymbolsClipboard &operator=(DisplaySymbolsClipboard &&) = delete;
    ~DisplaySymbolsClipboard() = default;

    static DisplaySymbolsClipboard &get_instance();

    DisplaySymbol symbol;
    std::map<int, DisplaySymbol> symbols;

    void reset_symbols();
    void set_symbol(const std::map<int, DisplaySymbol> &symbol_configs);

private:
    DisplaySymbolsClipboard();
    static DisplaySymbolsClipboard instance;
};

bool visual_mode_command(char ch, bool *visual_list_ptr, int height, int width,
    TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
    TERM_COLOR *cur_attr_ptr, char *cur_char_ptr, bool *need_redraw);

bool open_temporary_file(FILE **fff, char *file_name);
void browser_cursor(char ch, int *column, IDX *grp_cur, int grp_cnt, IDX *list_cur, int list_cnt);
void display_group_list(int wid, int per_page, const std::vector<short> &grp_idx, const std::vector<std::string> &group_text, int grp_cur, int grp_top);
void display_visual_list(int col, int row, int height, int width, TERM_COLOR attr_top, byte char_left);
void place_visual_list_cursor(TERM_LEN col, TERM_LEN row, TERM_COLOR a, byte c, TERM_COLOR attr_top, byte char_left);
