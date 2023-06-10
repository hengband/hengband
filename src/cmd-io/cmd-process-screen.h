#pragma once

#include <filesystem>

class PlayerType;
void exe_cmd_save_screen_html(const std::filesystem::path &path, bool need_message);
void do_cmd_save_screen(PlayerType *player_ptr);
void do_cmd_load_screen(void);
