#include "info-reader/monster-flag-reader.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/language-switcher.h"
#include "view/display-messages.h"

/*!
 * @brief フラグ名をモンスターの基本フラグの表で探し、見つかった表のフラグを1つ立てる
 * @details 耐性・オーラ・習性・シンボル・種族・ドロップ・荒野・地形・出現数・セリフ・明暗・特殊・その他の順に探す。
 * 見つからなかったときはどのフラグも変えず、未知のモンスター・フラグとして表示する。
 * 性別はモンスターの定義では別の項目で読み込み、ダンジョンの出現条件では呼び出し側で先に探すため、ここでは扱わない。
 * @param groups 基本フラグの格納先
 * @param what フラグ名
 * @return 見つけたら true
 */
bool grab_one_monster_basic_flag(const MonsterBasicFlagGroups &groups, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(*groups.resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterAuraType>::grab_one_flag(*groups.aura_flags, r_info_aura_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(*groups.behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(*groups.visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(*groups.kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(*groups.drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(*groups.wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(*groups.feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(*groups.population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(*groups.speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(*groups.brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(*groups.special_flags, r_info_special_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(*groups.misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}

/*!
 * @brief フラグ名を能力フラグ(魔法・ブレス)の表で探し、見つかったフラグを1つ立てる
 * @details 見つからなかったときはフラグを変えず、未知のモンスター・フラグとして表示する。
 * @param ability_flags 能力フラグの格納先
 * @param what フラグ名
 * @return 見つけたら true
 */
bool grab_one_monster_spell_flag(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, std::string_view what)
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}
