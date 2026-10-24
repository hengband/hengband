#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class PickType;
class PlayerType;
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes);
std::filesystem::path search_pickpref_path(std::string_view player_base_name);
std::pair<PickType, std::vector<std::unique_ptr<std::string>>> read_pickpref_text_lines(std::string_view player_base_name);
bool write_text_lines(std::string_view filename, const std::vector<std::string> &lines);
std::string pickpref_filename(std::string_view player_base_name, PickType filename_mode);
