#pragma once

#include "main-win/main-win-define.h"

#include <windows.h>

extern char bg_bitmap_file[MAIN_WIN_MAX_PATH]; //!< 現在の背景ビットマップファイル名。

void load_bg_prefs(void);
void finalize_bg();

void delete_bg(void);
BOOL init_bg(void);
void draw_bg(HDC hdc, RECT *r);
