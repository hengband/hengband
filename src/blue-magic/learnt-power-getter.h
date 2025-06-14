#pragma once
/*!
 * @file learnt-power-getter.h
 * @brief 青魔法の処理実行ヘッダ
 */

#include "system/angband.h"

#include <tl/optional.hpp>

enum class MonsterAbilityType;
class PlayerType;
struct monster_power;
int calculate_blue_magic_failure_probability(PlayerType *player_ptr, const monster_power &mp, int need_mana);
tl::optional<MonsterAbilityType> get_learned_power(PlayerType *player_ptr);
