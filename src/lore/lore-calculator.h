#pragma once

#include "system/angband.h"
#include <fmt/base.h>
#include <string>
#include <string_view>

enum class MonsterAbilityType;

struct lore_type;
class PlayerType;
std::string get_skill_damage(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type);
void add_lore_of_damage_skill(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, fmt::format_string<std::string_view> msg, byte color);
void set_flags_for_full_knowledge(lore_type *lore_ptr);
