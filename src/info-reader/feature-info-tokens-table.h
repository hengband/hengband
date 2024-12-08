/*!
 * @brief 地形特性トークン辞書定義
 * @author Hourier
 * @date 2024/12/08
 */

#pragma once

#include "grid/feature-flag-types.h"
#include "system/angband.h"
#include <string_view>
#include <unordered_map>

enum class TerrainTag;
extern const std::unordered_map<std::string_view, TerrainCharacteristics> f_info_flags;
extern const std::unordered_map<std::string_view, TerrainTag> terrain_tags;
