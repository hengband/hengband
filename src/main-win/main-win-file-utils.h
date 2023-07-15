#pragma once

#include "system/h-type.h"
#include <filesystem>
#include <initializer_list>
#include <string>

bool check_file(const std::filesystem::path &s);
bool check_dir(const std::filesystem::path &s);
std::string find_any_file(const std::filesystem::path &dir, std::initializer_list<concptr> files);
