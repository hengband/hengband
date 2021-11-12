#pragma once

#include "system/angband.h"
#include "grid/feature-flag-types.h"
#include <string_view>
#include <unordered_map>

extern const std::unordered_map<std::string_view, FloorFeatureType> f_info_flags;
