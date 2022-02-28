#pragma once

#include "grid/feature-flag-types.h"
#include "system/angband.h"
#include <string_view>
#include <unordered_map>

extern const std::unordered_map<std::string_view, FloorFeatureType> f_info_flags;
