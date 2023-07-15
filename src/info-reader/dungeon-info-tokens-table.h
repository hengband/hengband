#pragma once

#include "system/angband.h"

#include <string_view>
#include <unordered_map>

enum class DungeonFeatureType;

extern const std::unordered_map<std::string_view, DungeonFeatureType> dungeon_flags;
