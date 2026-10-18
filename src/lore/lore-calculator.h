#pragma once

#include "system/angband.h"
#include <fmt/base.h>
#include <string>
#include <string_view>

enum class MonsterAbilityType;
enum class MonraceId : short;

struct lore_type;
class PlayerType;
std::string dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div);
std::string get_skill_damage(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type);
void add_lore_of_damage_skill(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, fmt::format_string<std::string_view> msg, byte color);
void set_flags_for_full_knowledge(lore_type *lore_ptr);
