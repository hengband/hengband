#pragma once

#include "system/angband.h"
#include <string>

enum class MonsterAbilityType;
enum class MonsterRaceId : int16_t;

struct lore_type;
class PlayerType;
std::string dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div);
bool know_armour(MonsterRaceId r_idx, const bool know_everything);
bool know_damage(MonsterRaceId r_idx, int i);
void set_damage(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, concptr msg);
void set_drop_flags(lore_type *lore_ptr);
