#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

tl::optional<std::string> remove_auto_dump(const std::filesystem::path &orig_file, std::string_view auto_dump_mark);
