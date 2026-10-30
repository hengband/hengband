#pragma once

/*!
 * @file monster-flag-reader.h
 * @brief モンスターの定義と、ダンジョンのモンスターの出現条件に共通するフラグ名の読み込み
 */

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include <string_view>

bool grab_one_monster_spell_flag(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, std::string_view what);
