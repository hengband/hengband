#pragma once

#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-aura-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-flags9.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-visual-flags.h"
#include "system/angband.h"

#include <string_view>
#include <unordered_map>

enum class MonsterAbilityType;

extern const std::unordered_map<std::string_view, RaceBlowMethodType> r_info_blow_method;
extern const std::unordered_map<std::string_view, RaceBlowEffectType> r_info_blow_effect;
extern const std::unordered_map<std::string_view, race_flags1> r_info_flags1;
extern const std::unordered_map<std::string_view, race_flags2> r_info_flags2;
extern const std::unordered_map<std::string_view, race_flags3> r_info_flags3;
extern const std::unordered_map<std::string_view, MonsterAbilityType> r_info_ability_flags;
extern const std::unordered_map<std::string_view, race_flags7> r_info_flags7;
extern const std::unordered_map<std::string_view, race_flags8> r_info_flags8;
extern const std::unordered_map<std::string_view, race_flags9> r_info_flags9;
extern const std::unordered_map<std::string_view, MonsterResistanceType> r_info_flagsr;
extern const std::unordered_map<std::string_view, MonsterAuraType> r_info_aura_flags;
extern const std::unordered_map<std::string_view, MonsterBehaviorType> r_info_behavior_flags;
extern const std::unordered_map<std::string_view, MonsterVisualType> r_info_visual_flags;
extern const std::unordered_map<std::string_view, MonsterKindType> r_info_kind_flags;
