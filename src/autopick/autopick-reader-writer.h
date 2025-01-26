#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class PlayerType;
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes);
std::filesystem::path search_pickpref_path(std::string_view player_base_name);
std::vector<std::unique_ptr<std::string>> read_pickpref_text_lines(std::string_view player_base_name, int *filename_mode_p);
bool write_text_lines(std::string_view filename, const std::vector<std::unique_ptr<std::string>> &lines);
std::string pickpref_filename(std::string_view player_base_name, int filename_mode);
