#pragma once

#include "system/angband.h"

void move_cursor(int row, int col);
void flush(void);
void screen_save();
void screen_load();
void c_put_str(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
void put_str(concptr str, TERM_LEN row, TERM_LEN col);
void c_prt(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col);
void prt(concptr str, TERM_LEN row, TERM_LEN col);
void c_roff(TERM_COLOR attr, concptr str);
void roff(concptr str);
void clear_from(int row);
