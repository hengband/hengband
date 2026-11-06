#pragma once

#include <string_view>
#include <unordered_map>

enum class ItemKindType : short;
enum class PlayerClassType : short;
extern const std::unordered_map<std::string_view, ItemKindType> magic_info_spelltype_to_tval;
extern const std::unordered_map<std::string_view, int> magic_info_name_to_stat;
extern const std::unordered_map<std::string_view, PlayerClassType> magic_info_class_list;
