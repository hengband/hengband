#pragma once

#include "system/angband.h"
#include <string_view>

/** 画面情報保存スタックからの読み出しオプション */
enum class ScreenLoadOptType {
    ONE, //!< スタックの先頭のデータを1つ読み出す
    ALL, //!< スタックからすべてのデータを読み出す(画面は最後に読み出したデータになる)
};

void move_cursor(int row, int col);
void flush(void);
void screen_save();
void screen_load(ScreenLoadOptType opt = ScreenLoadOptType::ONE);
void c_put_str(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col);
void put_str(std::string_view sv, TERM_LEN row, TERM_LEN col);
void c_prt(TERM_COLOR attr, std::string_view sv, TERM_LEN row, TERM_LEN col);
void prt(std::string_view sv, TERM_LEN row, TERM_LEN col);
void c_roff(TERM_COLOR attr, std::string_view str);
void roff(std::string_view str);
void clear_from(int row);
