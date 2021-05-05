#pragma once

#include "system/angband.h"

#include <string_view>
#include <unordered_map>

enum class DF;

extern const std::unordered_map<std::string_view, DF> d_info_flags;
