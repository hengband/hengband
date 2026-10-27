/*!
 * @brief 地形定義トークン辞書定義
 * @author Hourier
 * @date 2024/12/08
 */

#pragma once

#include <string_view>
#include <unordered_map>

enum class TerrainCharacteristics;
enum class TerrainTag;
enum class TerrainConversionType;
enum class TrapType;
enum class PatternTileType;
enum class StoreSaleType;
enum class BuildingType;
extern const std::unordered_map<std::string_view, TerrainCharacteristics> f_info_flags;
extern const std::unordered_map<std::string_view, TerrainTag> terrain_tags;
extern const std::unordered_map<std::string_view, TerrainConversionType> f_info_conversion_types;
extern const std::unordered_map<std::string_view, TrapType> f_info_trap_types;
extern const std::unordered_map<std::string_view, PatternTileType> f_info_pattern_types;
extern const std::unordered_map<std::string_view, StoreSaleType> f_info_store_types;
extern const std::unordered_map<std::string_view, BuildingType> f_info_building_types;
