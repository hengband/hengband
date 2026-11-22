#pragma once

#include "system/h-basic.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

class PlayerType;
void process_player_name(PlayerType *player_ptr, bool is_new_savefile = false);
void get_name(PlayerType *player_ptr);
std::string make_player_base_name(std::string_view name);
std::string make_player_name_for_expression(std::string_view name);
#if defined(JP) && defined(UTF8)
tl::optional<std::filesystem::path> rename_legacy_named_file(const std::filesystem::path &path);
#endif
