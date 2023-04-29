#pragma once

#include <filesystem>
#include <string_view>

void remove_auto_dump(const std::filesystem::path &orig_file, std::string_view auto_dump_mark);
