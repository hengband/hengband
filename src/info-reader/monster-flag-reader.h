#pragma once

/*!
 * @file monster-flag-reader.h
 * @brief モンスターの定義と、ダンジョンのモンスターの出現条件に共通するフラグ名の読み込み
 */

#include "monster-race/monster-aura-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-population-flags.h"
#include "monster-race/race-speak-flags.h"
#include "monster-race/race-special-flags.h"
#include "monster-race/race-visual-flags.h"
#include "monster-race/race-wilderness-flags.h"
#include "util/flag-group.h"
#include <string_view>

/*!
 * @brief モンスターの基本フラグ(能力フラグ・性別以外)の格納先
 * @details モンスターの定義 (MonraceDefinition) と、ダンジョンのモンスターの出現条件 (DungeonDefinition の mon_*) のどちらにも向けられるよう、各フラグへのポインタを持つ。どれも null にしない。
 */
struct MonsterBasicFlagGroups {
    EnumClassFlagGroup<MonsterResistanceType> *resistance_flags;
    EnumClassFlagGroup<MonsterAuraType> *aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> *behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> *visual_flags;
    EnumClassFlagGroup<MonsterKindType> *kind_flags;
    EnumClassFlagGroup<MonsterDropType> *drop_flags;
    EnumClassFlagGroup<MonsterWildernessType> *wilderness_flags;
    EnumClassFlagGroup<MonsterFeatureType> *feature_flags;
    EnumClassFlagGroup<MonsterPopulationType> *population_flags;
    EnumClassFlagGroup<MonsterSpeakType> *speak_flags;
    EnumClassFlagGroup<MonsterBrightnessType> *brightness_flags;
    EnumClassFlagGroup<MonsterSpecialType> *special_flags;
    EnumClassFlagGroup<MonsterMiscType> *misc_flags;
};

bool grab_one_monster_basic_flag(const MonsterBasicFlagGroups &groups, std::string_view what);
bool grab_one_monster_spell_flag(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, std::string_view what);
