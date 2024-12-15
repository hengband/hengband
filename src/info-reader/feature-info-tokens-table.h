/*!
 * @brief 地形特性トークン辞書定義
 * @author Hourier
 * @date 2024/12/08
 */

#pragma once

#include <string_view>
#include <unordered_map>

enum class TerrainCharacteristics;
enum class TerrainTag;
extern const std::unordered_map<std::string_view, TerrainCharacteristics> f_info_flags;
extern const std::unordered_map<std::string_view, TerrainTag> terrain_tags;
