#pragma once

#include "main-win/main-win-define.h"
#include <filesystem>
#include <windows.h>

/*!
 * mode of background
 */
enum class bg_mode {
    BG_NONE = 0,
    BG_ONE = 1,
    BG_PRESET = 2,
};

void load_bg_prefs(void);
void finalize_bg();

void delete_bg(void);
bool load_bg(const std::filesystem::path &path);
void draw_bg(HDC hdc, RECT *r);
