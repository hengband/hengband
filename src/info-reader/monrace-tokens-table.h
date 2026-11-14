#pragma once

#include <string_view>
#include <unordered_map>

enum class MonsterAbilityType;
enum class MonsterAuraType;
enum class MonsterBehaviorType;
enum class MonsterBrightnessType;
enum class MonsterDropType;
enum class MonsterFeatureType;
enum class MonsterKindType;
enum class MonsterMiscType;
enum class MonsterPopulationType;
enum class MonsterResistanceType;
enum class MonsterSex;
enum class MonsterSpeakType;
enum class MonsterMessageType;
enum class MonsterSpecialType;
enum class MonsterVisualType;
enum class MonsterWildernessType;
enum class RaceBlowEffectType;
enum class RaceBlowMethodType;

extern const std::unordered_map<std::string_view, RaceBlowMethodType> monrace_blow_method;
extern const std::unordered_map<std::string_view, RaceBlowEffectType> monrace_blow_effect;
extern const std::unordered_map<std::string_view, MonsterAbilityType> monrace_ability_flags;
extern const std::unordered_map<std::string_view, MonsterResistanceType> monrace_resistance_flags;
extern const std::unordered_map<std::string_view, MonsterAuraType> monrace_aura_flags;
extern const std::unordered_map<std::string_view, MonsterBehaviorType> monrace_behavior_flags;
extern const std::unordered_map<std::string_view, MonsterVisualType> monrace_visual_flags;
extern const std::unordered_map<std::string_view, MonsterKindType> monrace_kind_flags;
extern const std::unordered_map<std::string_view, MonsterDropType> monrace_drop_flags;
extern const std::unordered_map<std::string_view, MonsterWildernessType> monrace_wilderness_flags;
extern const std::unordered_map<std::string_view, MonsterFeatureType> monrace_feature_flags;
extern const std::unordered_map<std::string_view, MonsterPopulationType> monrace_population_flags;
extern const std::unordered_map<std::string_view, MonsterSpeakType> monrace_speak_flags;
extern const std::unordered_map<std::string_view, MonsterMessageType> monrace_message_flags;
extern const std::unordered_map<std::string_view, MonsterBrightnessType> monrace_brightness_flags;
extern const std::unordered_map<std::string_view, MonsterSex> monrace_sex;
extern const std::unordered_map<std::string_view, MonsterSpecialType> monrace_special_flags;
extern const std::unordered_map<std::string_view, MonsterMiscType> monrace_misc_flags;
