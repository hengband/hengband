#include "info-reader/race-reader.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/character-encoding.h"
#include "player-ability/player-ability-types.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "term/gameterm.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <nlohmann/json.hpp>
#include <string>

RaceReader::RaceReader(const nlohmann::json &monrace_data)
    : monrace_data(monrace_data)
{
}

/*!
 * @brief モンスター種族情報(MonraceDefinitions)のパース関数
 * @return エラーコード
 */
int RaceReader::read() const
{
    if (!this->monrace_data.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto &id_obj = get_json_value(this->monrace_data, "id");
    if (!id_obj.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto monrace_id_int = id_obj.get<int>();
    if (monrace_id_int < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }

    error_idx = monrace_id_int;
    const auto monrace_id = i2enum<MonraceId>(monrace_id_int);
    auto &monraces = MonraceList::get_instance();
    auto &monrace = monraces.emplace(monrace_id);
    monrace.idx = monrace_id;

    errr err;
    err = this->set_mon_name(monrace);
    if (err) {
        msg_format(_("モンスター名読込失敗。ID: '%d'。", "Failed to load monster name. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_symbol(monrace);
    if (err) {
        msg_format(_("モンスターシンボル読込失敗。ID: '%d'。", "Failed to load monster symbol. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_speed(monrace);
    if (err) {
        msg_format(_("モンスター速度読込失敗。ID: '%d'。", "Failed to load monster speed. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_dice(get_json_value(this->monrace_data, "hit_point"), monrace.hit_dice, true);
    if (err) {
        msg_format(_("モンスターHP読込失敗。ID: '%d'。", "Failed to load monster HP. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "vision"), monrace.aaf, true, Range(0, 999));
    if (err) {
        msg_format(_("モンスター感知範囲読込失敗。ID: '%d'。", "Failed to load monster vision. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "armor_class"), monrace.ac, true, Range(0, 10000));
    if (err) {
        msg_format(_("モンスターAC読込失敗。ID: '%d'。", "Failed to load monster AC. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "alertness"), monrace.sleep, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスター警戒度読込失敗。ID: '%d'。", "Failed to load monster alertness. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "level"), monrace.level, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスターレベル読込失敗。ID: '%d'。", "Failed to load monster level. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "rarity"), monrace.rarity, true, Range(0, 255));
    if (err) {
        msg_format(_("モンスター希少度読込失敗。ID: '%d'。", "Failed to load monster rarity. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "exp"), monrace.mexp, true, Range(0, 9999999));
    if (err) {
        msg_format(_("モンスター経験値読込失敗。ID: '%d'。", "Failed to load monster exp. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_evolve(monrace);
    if (err) {
        msg_format(_("モンスター進化情報読込失敗。ID: '%d'。", "Failed to load monster evolve data. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_sex(monrace);
    if (err) {
        msg_format(_("モンスター性別読込失敗。ID: '%d'。", "Failed to load monster sex. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "odds_correction_ratio"), monrace.arena_ratio, false, Range(1, 9999));
    if (err) {
        msg_format(_("モンスター賭け倍率読込失敗。ID: '%d'。", "Failed to load monster odds for arena. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_integer(get_json_value(this->monrace_data, "start_hp_percentage"), monrace.cur_hp_per, false, Range(0, 99));
    if (err) {
        msg_format(_("モンスター初期体力読込失敗。ID: '%d'。", "Failed to load monster starting HP. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_artifacts(monrace);
    if (err) {
        msg_format(_("モンスター固定アーティファクトドロップ情報読込失敗。ID: '%d'。", "Failed to load monster artifact drop data. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_escorts(monrace);
    if (err) {
        msg_format(_("モンスター護衛情報読込失敗。ID: '%d'。", "Failed to load monster escorts. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_blows(monrace);
    if (err) {
        msg_format(_("モンスター打撃情報読込失敗。ID: '%d'。", "Failed to load monster blow data. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_flags(monrace);
    if (err) {
        msg_format(_("モンスターフラグ読込失敗。ID: '%d'。", "Failed to load monster flag data. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_skills(monrace);
    if (err) {
        msg_format(_("モンスター発動能力情報読込失敗。ID: '%d'。", "Failed to load monster skill data. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_final_summons(monrace);
    if (err) {
        msg_format(_("モンスター死亡時召喚情報読込失敗。ID: '%d'。", "Failed to load final summon data. ID: '%d'."), error_idx);
        return err;
    }
    err = info_set_string(get_json_value(this->monrace_data, "flavor"), monrace.text, false);
    if (err) {
        msg_format(_("モンスター説明文読込失敗。ID: '%d'。", "Failed to load monster flavor text. ID: '%d'."), error_idx);
        return err;
    }
    err = this->set_mon_message(monrace);
    if (err) {
        msg_format(_("モンスターメッセージ読込失敗。ID: '%d'。", "Failed to load monster message. ID: '%d'."), error_idx);
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用1) /
 * Grab one (basic) flag in a MonraceDefinition from a textual string
 * @param monrace 保管先のモンスター種族構造体
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool RaceReader::grab_one_basic_flag(MonraceDefinition &monrace, std::string_view what) const
{
    if (EnumClassFlagGroup<MonsterResistanceType>::grab_one_flag(monrace.resistance_flags, r_info_flagsr, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterAuraType>::grab_one_flag(monrace.aura_flags, r_info_aura_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBehaviorType>::grab_one_flag(monrace.behavior_flags, r_info_behavior_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterVisualType>::grab_one_flag(monrace.visual_flags, r_info_visual_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterKindType>::grab_one_flag(monrace.kind_flags, r_info_kind_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterDropType>::grab_one_flag(monrace.drop_flags, r_info_drop_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterWildernessType>::grab_one_flag(monrace.wilderness_flags, r_info_wilderness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterFeatureType>::grab_one_flag(monrace.feature_flags, r_info_feature_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterPopulationType>::grab_one_flag(monrace.population_flags, r_info_population_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpeakType>::grab_one_flag(monrace.speak_flags, r_info_speak_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterBrightnessType>::grab_one_flag(monrace.brightness_flags, r_info_brightness_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<MonsterSpecialType>::grab_one_flag(monrace.special_flags, r_info_special_flags, what)) {
        return true;
    }
    if (EnumClassFlagGroup<MonsterMiscType>::grab_one_flag(monrace.misc_flags, r_info_misc_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(モンスター用2) /
 * Grab one (spell) flag in a MonraceDefinition from a textual string
 * @param monrace 保管先のモンスター種族構造体
 * @param what 参照元の文字列
 * @return 見つけたらtrue
 */
bool RaceReader::grab_one_spell_flag(MonraceDefinition &monrace, std::string_view what) const
{
    if (EnumClassFlagGroup<MonsterAbilityType>::grab_one_flag(monrace.ability_flags, r_info_ability_flags, what)) {
        return true;
    }

    msg_format(_("未知のモンスター・フラグ '%s'。", "Unknown monster flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectからモンスター名をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_name(MonraceDefinition &monrace) const
{
    const auto &name_data = get_json_value(this->monrace_data, "name");
    if (name_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto &ja_obj = get_json_value(name_data, "ja");
    const auto &en_obj = get_json_value(name_data, "en");
    if (!ja_obj.is_string() || !en_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto ja_name = ja_obj.get<std::string>();
    const auto en_name = en_obj.get<std::string>();

#ifdef JP
    auto ja_name_sys = utf8_to_sys(ja_name);
    if (!ja_name_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    monrace.name = { *ja_name_sys, en_name };
#else
    monrace.name = { "", en_name };
#endif
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターシンボルをセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_symbol(MonraceDefinition &monrace) const
{
    const auto &symbol_data = get_json_value(this->monrace_data, "symbol");
    if (symbol_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &character_obj = get_json_value(symbol_data, "character");
    if (!character_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &color_obj = get_json_value(symbol_data, "color");
    if (!color_obj.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    if (color->second > 127) {
        return PARSE_ERROR_GENERIC;
    }

    monrace.symbol_definition = { color->second, character_obj.get<std::string>().front() };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスター速度をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_speed(MonraceDefinition &monrace) const
{
    int speed;
    if (auto err = info_set_integer(get_json_value(this->monrace_data, "speed"), speed, true, Range(-50, 99))) {
        return err;
    }
    monrace.speed = speed + STANDARD_SPEED;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの進化をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_evolve(MonraceDefinition &monrace) const
{
    const auto &evolve_data = get_json_value(this->monrace_data, "evolve");
    if (evolve_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

    if (auto err = info_set_integer(get_json_value(evolve_data, "need_exp"), monrace.next_exp, true, Range(0, 9999999))) {
        return err;
    }
    if (auto err = info_set_integer(get_json_value(evolve_data, "to"), monrace.next_r_idx, true, Range(0, 9999))) {
        return err;
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの性別をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_sex(MonraceDefinition &monrace) const
{
    const auto &sex_data = get_json_value(this->monrace_data, "sex");
    if (sex_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!sex_data.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    uint32_t sex;
    if (!info_grab_one_const(sex, r_info_sex, sex_data.get<std::string>())) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    monrace.init_sex(sex);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの固定アーティファクトドロップ情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_artifacts(MonraceDefinition &monrace) const
{
    const auto &artifact_data = get_json_value(this->monrace_data, "artifacts");
    if (artifact_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!artifact_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &artifact : artifact_data) {
        FixedArtifactId fa_id;
        if (auto err = info_set_integer(get_json_value(artifact, "drop_artifact_id"), fa_id, true, Range(0, 1024))) {
            return err;
        }
        int chance;
        if (auto err = info_set_integer(get_json_value(artifact, "drop_probability"), chance, true, Range(1, 100))) {
            return err;
        }

        monrace.emplace_drop_artifact(fa_id, chance);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの護衛情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_escorts(MonraceDefinition &monrace) const
{
    const auto &escort_data = get_json_value(this->monrace_data, "escorts");
    if (escort_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!escort_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &escort : escort_data) {
        MonraceId monrace_id;
        if (auto err = info_set_integer(get_json_value(escort, "escorts_id"), monrace_id, true, Range(0, 8192))) {
            return err;
        }

        Dice dice;
        if (auto err = info_set_dice(get_json_value(escort, "escort_num"), dice, true)) {
            return err;
        }

        monrace.emplace_reinforce(monrace_id, dice);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの打撃攻撃をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_blows(MonraceDefinition &monrace) const
{
    const auto &blow_data = get_json_value(this->monrace_data, "blows");
    if (blow_data.is_null()) {
        monrace.behavior_flags.set(MonsterBehaviorType::NEVER_BLOW);
        return PARSE_ERROR_NONE;
    }
    if (!blow_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto blow_num = 0; const auto &blow : blow_data) {
        if (blow_num > 5) {
            return PARSE_ERROR_GENERIC;
        }

        const auto &blow_method = get_json_value(blow, "method");
        const auto &blow_effect = get_json_value(blow, "effect");
        if (blow_method.is_null() || blow_effect.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        const auto rbm = r_info_blow_method.find(blow_method.get<std::string>());
        if (rbm == r_info_blow_method.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        const auto rbe = r_info_blow_effect.find(blow_effect.get<std::string>());
        if (rbe == r_info_blow_effect.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        auto &mon_blow = monrace.blows[blow_num];
        mon_blow.method = rbm->second;
        mon_blow.effect = rbe->second;

        if (auto err = info_set_dice(get_json_value(blow, "damage_dice"), mon_blow.damage_dice, false)) {
            return err;
        }

        blow_num++;
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターフラグをセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_flags(MonraceDefinition &monrace) const
{
    const auto &flag_data = get_json_value(this->monrace_data, "flags");
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &flag : flag_data) {
        if (!this->grab_one_basic_flag(monrace, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの発動能力をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_skills(MonraceDefinition &monrace) const
{
    const auto &skill_data = get_json_value(this->monrace_data, "skill");
    if (skill_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!skill_data.is_object()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &prob = get_json_value(skill_data, "probability");
    if (!prob.is_string()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &prob_token = str_split(prob.get<std::string>(), '_', false, 2);
    if (prob_token.size() == 3 && prob_token[1] == "IN") {
        if (prob_token[0] != "1") {
            return PARSE_ERROR_GENERIC;
        }
        byte denominator;
        info_set_value(denominator, prob_token[2]);
        monrace.freq_spell = 100 / denominator;
    }

    const auto &shoot_dice = skill_data.find("shoot");
    const auto shoot = (shoot_dice != skill_data.end());
    if (shoot) {
        if (auto ret = info_set_dice(shoot_dice->get<std::string>(), monrace.shoot_damage_dice, true)) {
            return ret;
        }
        monrace.ability_flags.set(MonsterAbilityType::SHOOT);
    }

    const auto &skill_list = skill_data.find("list");
    if (skill_list == skill_data.end()) {
        if (!shoot) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        return PARSE_ERROR_NONE;
    }

    for (auto &skill : skill_list->items()) {
        if (!this->grab_one_spell_flag(monrace, skill.value().get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターの死亡時召喚情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_final_summons(MonraceDefinition &monrace) const
{
    const auto &summon_data = get_json_value(this->monrace_data, "final_summon");
    if (summon_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!summon_data.is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    for (auto &summon_item : summon_data) {
        int id, probability, min_num, max_num, radius;
        if (auto err = info_set_integer(get_json_value(summon_item, "id"), id, true, Range(1, 9999))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(summon_item, "probability"), probability, true, Range(1, 100))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(summon_item, "min_num"), min_num, true, Range(0, 99))) {
            return err;
        }
        if (auto err = info_set_integer(get_json_value(summon_item, "max_num"), max_num, true, Range(1, 99))) {
            return err;
        }
        if (min_num > max_num) {
            return PARSE_ERROR_INVALID_VALUE;
        }
        if (auto err = info_set_integer(get_json_value(summon_item, "radius"), radius, true, Range(1, 20))) {
            return err;
        }
        monrace.emplace_final_summon(i2enum<MonraceId>(id), probability, min_num, max_num, radius);
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return エラーコード
 */
int RaceReader::set_mon_message(MonraceDefinition &monrace) const
{
    const auto &message_data = get_json_value(this->monrace_data, "message");
    if (message_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!message_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (const auto &message : message_data) {
        const auto &action_str = get_json_value(message, "action");
        if (action_str.is_null()) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }
        const auto action = r_info_message_flags.find(action_str.get<std::string>());
        if (action == r_info_message_flags.end()) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        int chance;
        if (auto err = info_set_integer(get_json_value(message, "chance"), chance, true, Range(1, 100))) {
            return err;
        }

        bool use_name = true;
        const auto use_name_iter = message.find("use_name");
        if (use_name_iter != message.end()) {
            const auto &use_name_data = use_name_iter.value();
            if (auto err = info_set_bool(use_name_data, use_name, false)) {
                return err;
            }
        }

        std::string str;
        if (auto err = info_set_string(get_json_value(message, "message"), str, false)) {
            return err;
        }

        MonraceMessageList::get_instance().emplace((int)monrace.idx, action->second, chance, use_name, str);
    }
    return PARSE_ERROR_NONE;
}
