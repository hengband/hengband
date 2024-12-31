#pragma once

#include "system/angband.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class PlayerType;
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes);
std::filesystem::path search_pickpref_path(std::string_view player_base_name);
std::vector<concptr> read_pickpref_text_lines(std::string_view player_base_name, int *filename_mode_p);
bool write_text_lines(std::string_view filename, const std::vector<concptr> &lines);
std::string pickpref_filename(std::string_view player_base_name, int filename_mode);
