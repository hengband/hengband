#include "info-reader/race-reader.h"
#include "artifact/fixed-art-types.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/race-info-tokens-table.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "player-ability/player-ability-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-message.h"
#include "term/gameterm.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <charconv>
#include <cstdint>
#include <fmt/format.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <system_error>
#include <utility>

namespace {
//! モンスター定義のダイスの最大の出目(個数×面数)の上限。捕獲したモンスターのHPを保持するshortに収める。
constexpr int64_t MAX_MONRACE_DICE_ROLL = std::numeric_limits<short>::max();

/*!
 * @brief モンスターのACの上限
 * @details shoot.cpp の test_hit_fire() は、起きている五右衛門の AC を3倍して ARMOUR_CLASS (16ビット) に入れる。
 * 3倍しても 32,767 に収まるのは 10,922 までなので、同梱のデータの最大 (10,000) と同じ 10,000 を上限とする。
 * ほかの AC を使う計算 (命中率・モンスター同士の攻撃) は int で行うため、これより緩い。
 */
constexpr int MONRACE_AC_MAX = 10000;

//! 闘技場のオッズ補正は100で等倍。10倍あれば十分で、同梱データの最大値900も収まる。
constexpr int MONRACE_ODDS_CORRECTION_RATIO_MAX = 1000;
//! 進化に必要な経験値 (need_exp、格納先は next_exp) が0だと進化しないため、進化を定義する場合は1以上とする。
constexpr int MONRACE_EVOLVE_NEED_EXP_MIN = 1;

/*!
 * @brief JSON Objectからモンスター定義のダイスを読み、値の範囲を検証する
 * @param json ダイスを表す文字列が格納されたJSON Object
 * @param dice 保管先のダイス
 * @param is_required 必須かどうか
 * @return 成功、またはパースエラー
 * @details 個数と面数は1以上、最大の出目は MAX_MONRACE_DICE_ROLL 以下とする。
 * 範囲外の値と "01d5" のような先頭に0の付いた表記は ParseErrorType::INVALID_VALUE を返し、保管先を変更しない。
 */
tl::expected<void, ParseErrorType> info_set_monrace_dice(const nlohmann::json &json, Dice &dice, bool is_required)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }

    Dice parsed;
    if (const auto result = info_set_dice(json, parsed, true); !result) {
        return result;
    }
    if (!parsed.is_valid() || static_cast<int64_t>(parsed.num) * parsed.sides > MAX_MONRACE_DICE_ROLL) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }
    // CI の pattern と同じく、先頭に0の付いた個数や面数を拒否する。
    if (parsed.to_string() != json.get<std::string>()) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    dice = parsed;
    return {};
}
}

RaceReader::Session RaceReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, MAX_MONRACE_ID }, _("モンスター種族", "monster race"), last_id);
}

RaceReader::RaceReader(const nlohmann::json &monrace_data, Session &session)
    : monrace_data(monrace_data)
    , session(session)
{
}

/*!
 * @brief モンスター種族情報(MonraceDefinitions)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> RaceReader::read() const
{
    if (!this->monrace_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto id_result = this->session.read(get_json_value(this->monrace_data, "id"));
    if (!id_result) {
        msg_print(id_result.error().reason);
        return tl::unexpected(id_result.error().code);
    }
    const auto monrace_id_int = *id_result;
    const auto monrace_id = i2enum<MonraceId>(monrace_id_int);
    auto &monraces = MonraceList::get_instance();
    auto monrace = monraces.contains(monrace_id) ? MonraceDefinition(monraces.get_monrace(monrace_id)) : MonraceDefinition();
    monrace.idx = monrace_id;

    if (const auto result = this->set_mon_name(monrace); !result) {
        msg_print(_("モンスター名読込失敗。", "Failed to load monster name."));
        return result;
    }
    if (const auto result = this->set_mon_symbol(monrace); !result) {
        msg_print(_("モンスターシンボル読込失敗。", "Failed to load monster symbol."));
        return result;
    }
    if (const auto result = this->set_mon_speed(monrace); !result) {
        msg_print(_("モンスター速度読込失敗。", "Failed to load monster speed."));
        return result;
    }
    if (const auto result = info_set_monrace_dice(get_json_value(this->monrace_data, "hit_point"), monrace.hit_dice, true); !result) {
        msg_print(_("モンスターHP読込失敗。", "Failed to load monster HP."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "vision"), monrace.aaf, true, Range(0, 999)); !result) {
        msg_print(_("モンスター感知範囲読込失敗。", "Failed to load monster vision."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "armor_class"), monrace.ac, true, Range(0, MONRACE_AC_MAX)); !result) {
        msg_print(_("モンスターAC読込失敗。", "Failed to load monster AC."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "alertness"), monrace.sleep, true, Range(0, 255)); !result) {
        msg_print(_("モンスター警戒度読込失敗。", "Failed to load monster alertness."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "level"), monrace.level, true, Range(0, MAX_DEPTH - 1)); !result) {
        msg_print(_("モンスターレベル読込失敗。", "Failed to load monster level."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "rarity"), monrace.rarity, true, Range(1, 255)); !result) {
        msg_print(_("モンスター希少度読込失敗。", "Failed to load monster rarity."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "exp"), monrace.mexp, true, Range(0, 9999999)); !result) {
        msg_print(_("モンスター経験値読込失敗。", "Failed to load monster exp."));
        return result;
    }
    if (const auto result = this->set_mon_evolve(monrace); !result) {
        msg_print(_("モンスター進化情報読込失敗。", "Failed to load monster evolve data."));
        return result;
    }
    if (const auto result = this->set_mon_sex(monrace); !result) {
        msg_print(_("モンスター性別読込失敗。", "Failed to load monster sex."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "odds_correction_ratio"), monrace.arena_ratio, false, Range(1, MONRACE_ODDS_CORRECTION_RATIO_MAX)); !result) {
        msg_print(_("モンスター賭け倍率読込失敗。", "Failed to load monster odds for arena."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "start_hp_percentage"), monrace.cur_hp_per, false, Range(1, 99)); !result) {
        msg_print(_("モンスター初期体力読込失敗。", "Failed to load monster starting HP."));
        return result;
    }
    if (const auto result = this->set_mon_artifacts(monrace); !result) {
        msg_print(_("モンスター固定アーティファクトドロップ情報読込失敗。", "Failed to load monster artifact drop data."));
        return result;
    }
    if (const auto result = this->set_mon_escorts(monrace); !result) {
        msg_print(_("モンスター護衛情報読込失敗。", "Failed to load monster escorts."));
        return result;
    }
    if (const auto result = this->set_mon_blows(monrace); !result) {
        msg_print(_("モンスター打撃情報読込失敗。", "Failed to load monster blow data."));
        return result;
    }
    if (const auto result = this->set_mon_flags(monrace); !result) {
        msg_print(_("モンスターフラグ読込失敗。", "Failed to load monster flag data."));
        return result;
    }
    if (const auto result = this->set_mon_skills(monrace); !result) {
        msg_print(_("モンスター発動能力情報読込失敗。", "Failed to load monster skill data."));
        return result;
    }
    if (const auto result = this->set_mon_final_summons(monrace); !result) {
        msg_print(_("モンスター死亡時召喚情報読込失敗。", "Failed to load final summon data."));
        return result;
    }
    if (const auto result = info_set_string(get_json_value(this->monrace_data, "flavor"), monrace.text, false); !result) {
        msg_print(_("モンスター説明文読込失敗。", "Failed to load monster flavor text."));
        return result;
    }
    auto &message_list = MonraceMessageList::get_instance();
    const auto message_it = message_list.messages.find(monrace_id_int);
    auto messages = message_it != message_list.messages.end() ? message_it->second : MonraceMessage();
    if (const auto result = this->set_mon_message(messages); !result) {
        msg_print(_("モンスターメッセージ読込失敗。", "Failed to load monster message."));
        return result;
    }

    // Parse failures do not publish either definition fields or partial message groups.
    // Allocation failures are not parse errors and are not covered by this guarantee.
    const auto &message_data = get_json_value(this->monrace_data, "message");
    if (!message_data.is_null() && !message_data.empty()) {
        message_list.messages.insert_or_assign(monrace_id_int, std::move(messages));
    }
    auto &published = monraces.contains(monrace_id) ? monraces.get_monrace(monrace_id) : monraces.emplace(monrace_id);
    published = std::move(monrace);
    return {};
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

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
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

    msg_print(_("未知のモンスター・フラグ '{}'。", "Unknown monster flag '{}'."), what);
    return false;
}

/*!
 * @brief JSON Objectからモンスター名をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_name(MonraceDefinition &monrace) const
{
    const auto &name_data = get_json_value(this->monrace_data, "name");
    if (name_data.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    const auto &ja_obj = get_json_value(name_data, "ja");
    const auto &en_obj = get_json_value(name_data, "en");
    if (!ja_obj.is_string() || !en_obj.is_string()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto ja_name = ja_obj.get<std::string>();
    const auto en_name = en_obj.get<std::string>();

#ifdef JP
    auto ja_name_sys = utf8_to_sys(ja_name);
    if (!ja_name_sys) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }
    monrace.name = { *ja_name_sys, en_name };
#else
    monrace.name = { "", en_name };
#endif
    return {};
}

/*!
 * @brief JSON Objectからモンスターシンボルをセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_symbol(MonraceDefinition &monrace) const
{
    const auto &symbol_data = get_json_value(this->monrace_data, "symbol");
    if (symbol_data.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &character_obj = get_json_value(symbol_data, "character");
    if (!character_obj.is_string()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &color_obj = get_json_value(symbol_data, "color");
    if (!color_obj.is_string()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }
    if (color->second > 127) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto character = character_obj.get<std::string>();
    if (character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    monrace.symbol_definition = { color->second, character.front() };
    return {};
}

/*!
 * @brief JSON Objectからモンスター速度をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_speed(MonraceDefinition &monrace) const
{
    int speed = 0;
    if (const auto result = info_set_integer(get_json_value(this->monrace_data, "speed"), speed, true, Range(-50, 99)); !result) {
        return result;
    }
    monrace.speed = speed + STANDARD_SPEED;
    return {};
}

/*!
 * @brief JSON Objectからモンスターの進化をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_evolve(MonraceDefinition &monrace) const
{
    const auto &evolve_data = get_json_value(this->monrace_data, "evolve");
    if (evolve_data.is_null()) {
        return {};
    }

    if (const auto result = info_set_integer(get_json_value(evolve_data, "need_exp"), monrace.next_exp, true, Range(MONRACE_EVOLVE_NEED_EXP_MIN, 9999999)); !result) {
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(evolve_data, "to"), monrace.next_r_idx, true, Range(1, MAX_MONRACE_ID)); !result) {
        return result;
    }

    return {};
}

/*!
 * @brief JSON Objectからモンスターの性別をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_sex(MonraceDefinition &monrace) const
{
    const auto &sex_data = get_json_value(this->monrace_data, "sex");
    if (sex_data.is_null()) {
        return {};
    }
    if (!sex_data.is_string()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    uint32_t sex;
    if (!info_grab_one_const(sex, r_info_sex, sex_data.get<std::string>())) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }
    monrace.init_sex(sex);
    return {};
}

/*!
 * @brief JSON Objectからモンスターの固定アーティファクトドロップ情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_artifacts(MonraceDefinition &monrace) const
{
    const auto &artifact_data = get_json_value(this->monrace_data, "artifacts");
    if (artifact_data.is_null()) {
        return {};
    }
    if (!artifact_data.is_array()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (const auto &artifact : artifact_data) {
        FixedArtifactId fa_id{};
        if (const auto result = info_set_integer(get_json_value(artifact, "drop_artifact_id"), fa_id, true, Range(1, MAX_FIXED_ARTIFACT_ID)); !result) {
            return result;
        }
        int chance = 0;
        if (const auto result = info_set_integer(get_json_value(artifact, "drop_probability"), chance, true, Range(1, 100)); !result) {
            return result;
        }

        monrace.emplace_drop_artifact(fa_id, chance);
    }
    return {};
}

/*!
 * @brief JSON Objectからモンスターの護衛情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_escorts(MonraceDefinition &monrace) const
{
    const auto &escort_data = get_json_value(this->monrace_data, "escorts");
    if (escort_data.is_null()) {
        return {};
    }
    if (!escort_data.is_array()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (const auto &escort : escort_data) {
        MonraceId monrace_id{};
        if (const auto result = info_set_integer(get_json_value(escort, "escorts_id"), monrace_id, true, Range(1, MAX_MONRACE_ID)); !result) {
            return result;
        }

        Dice dice;
        if (const auto result = info_set_monrace_dice(get_json_value(escort, "escort_num"), dice, true); !result) {
            return result;
        }

        monrace.emplace_reinforce(monrace_id, dice);
    }
    return {};
}

/*!
 * @brief JSON Objectからモンスターの打撃攻撃をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_blows(MonraceDefinition &monrace) const
{
    const auto &blow_data = get_json_value(this->monrace_data, "blows");
    if (blow_data.is_null()) {
        monrace.behavior_flags.set(MonsterBehaviorType::NEVER_BLOW);
        return {};
    }
    if (!blow_data.is_array()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (auto blow_num = 0; const auto &blow : blow_data) {
        if (blow_num >= MAX_NUM_BLOWS) {
            return tl::unexpected(ParseErrorType::INVALID_VALUE);
        }

        const auto &blow_method = get_json_value(blow, "method");
        const auto &blow_effect = get_json_value(blow, "effect");
        if (blow_method.is_null() || blow_effect.is_null()) {
            return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!blow_method.is_string() || !blow_effect.is_string()) {
            return tl::unexpected(ParseErrorType::INVALID_TYPE);
        }

        const auto rbm = r_info_blow_method.find(blow_method.get<std::string>());
        if (rbm == r_info_blow_method.end()) {
            return tl::unexpected(ParseErrorType::INVALID_FLAG);
        }

        const auto rbe = r_info_blow_effect.find(blow_effect.get<std::string>());
        if (rbe == r_info_blow_effect.end()) {
            return tl::unexpected(ParseErrorType::INVALID_FLAG);
        }
        auto &mon_blow = monrace.blows[blow_num];
        mon_blow.method = rbm->second;
        mon_blow.effect = rbe->second;

        if (const auto result = info_set_monrace_dice(get_json_value(blow, "damage_dice"), mon_blow.damage_dice, false); !result) {
            return result;
        }

        blow_num++;
    }
    return {};
}

/*!
 * @brief JSON Objectからモンスターフラグをセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_flags(MonraceDefinition &monrace) const
{
    const auto &flag_data = get_json_value(this->monrace_data, "flags");
    if (flag_data.is_null()) {
        return {};
    }
    return info_read_flag_array(flag_data, [this, &monrace](const auto &flag) { return this->grab_one_basic_flag(monrace, flag.template get<std::string>()); }, ParseErrorType::TOO_FEW_ARGUMENTS, ParseErrorType::INVALID_TYPE);
}

/*!
 * @brief JSON Objectからモンスターの発動能力をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_skills(MonraceDefinition &monrace) const
{
    const auto &skill_data = get_json_value(this->monrace_data, "skill");
    if (skill_data.is_null()) {
        return {};
    }
    if (!skill_data.is_object()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &prob = get_json_value(skill_data, "probability");
    if (!prob.is_string()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto &prob_token = str_split(prob.get<std::string>(), '_', false, 2);
    if (prob_token.size() != 3 || prob_token[0] != "1" || prob_token[1] != "IN") {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }
    const auto &denominator_token = prob_token[2];
    int denominator;
    const auto [end, ec] = std::from_chars(denominator_token.data(), denominator_token.data() + denominator_token.size(), denominator);
    if (ec != std::errc{} || end != denominator_token.data() + denominator_token.size() || denominator_token.front() == '0' || denominator < 1 || denominator > 100) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }
    monrace.freq_spell = 100 / denominator;

    const auto &shoot_dice = skill_data.find("shoot");
    const auto shoot = (shoot_dice != skill_data.end());
    if (shoot) {
        if (const auto result = info_set_monrace_dice(*shoot_dice, monrace.shoot_damage_dice, true); !result) {
            return result;
        }
        monrace.ability_flags.set(MonsterAbilityType::SHOOT);
    }

    const auto &skill_list = skill_data.find("list");
    if ((skill_list == skill_data.end()) || skill_list->is_null()) {
        if (!shoot) {
            return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }

    return info_read_flag_array(*skill_list, [this, &monrace](const nlohmann::json &flag) { return this->grab_one_spell_flag(monrace, flag.get<std::string>()); }, ParseErrorType::INVALID_TYPE, ParseErrorType::INVALID_TYPE);
}

/*!
 * @brief JSON Objectからモンスターの死亡時召喚情報をセットする
 * @param monrace 保管先のモンスター種族構造体
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_final_summons(MonraceDefinition &monrace) const
{
    const auto &summon_data = get_json_value(this->monrace_data, "final_summon");
    if (summon_data.is_null()) {
        return {};
    }
    if (!summon_data.is_array()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (auto &summon_item : summon_data) {
        int id, probability, min_num, max_num, radius;
        if (const auto result = info_set_integer(get_json_value(summon_item, "id"), id, true, Range(1, MAX_MONRACE_ID)); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(summon_item, "probability"), probability, true, Range(1, 100)); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(summon_item, "min_num"), min_num, true, Range(0, 99)); !result) {
            return result;
        }
        if (const auto result = info_set_integer(get_json_value(summon_item, "max_num"), max_num, true, Range(1, 99)); !result) {
            return result;
        }
        if (min_num > max_num) {
            return tl::unexpected(ParseErrorType::INVALID_VALUE);
        }
        if (const auto result = info_set_integer(get_json_value(summon_item, "radius"), radius, true, Range(1, 20)); !result) {
            return result;
        }
        monrace.emplace_final_summon(i2enum<MonraceId>(id), probability, min_num, max_num, radius);
    }
    return {};
}

/*!
 * @brief JSON Objectからモンスターのメッセージをセットする
 * @param messages 保管先の未公開メッセージ群
 * @return 成功、またはパースエラー
 */
tl::expected<void, ParseErrorType> RaceReader::set_mon_message(MonraceMessage &messages) const
{
    const auto &message_data = get_json_value(this->monrace_data, "message");
    if (message_data.is_null()) {
        return {};
    }
    if (!message_data.is_array()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    for (const auto &message : message_data) {
        const auto &action_str = get_json_value(message, "action");
        if (action_str.is_null()) {
            return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!action_str.is_string()) {
            return tl::unexpected(ParseErrorType::INVALID_TYPE);
        }
        const auto action = r_info_message_flags.find(action_str.get<std::string>());
        if (action == r_info_message_flags.end()) {
            return tl::unexpected(ParseErrorType::INVALID_FLAG);
        }

        int chance = 0;
        if (const auto result = info_set_integer(get_json_value(message, "chance"), chance, true, Range(1, 100)); !result) {
            return result;
        }

        bool use_name = true;
        const auto use_name_iter = message.find("use_name");
        if (use_name_iter != message.end()) {
            const auto &use_name_data = use_name_iter.value();
            if (const auto result = info_set_bool(use_name_data, use_name, false); !result) {
                return result;
            }
        }

        std::string str;
        if (const auto result = info_set_string(get_json_value(message, "message"), str, false); !result) {
            return result;
        }

        messages.emplace(action->second, chance, use_name, str);
    }
    return {};
}

/*!
 * @brief 読み込んだ全モンスター種族の参照先が定義されているかを検証する
 * @return 未定義の参照があればその説明、無ければ std::nullopt
 * @details 進化先・護衛・死亡時召喚は後ろのIDも参照できるため、全種族を読み込んだ後に呼ぶ。
 * 固定アーティファクトはモンスター種族より先に読み込まれている。
 */
std::optional<std::string> RaceReader::validate_references()
{
    const auto &monraces = MonraceList::get_instance();
    const auto &artifacts = ArtifactList::get_instance();
    const auto is_defined = [&monraces](MonraceId monrace_id) {
        return MonraceList::is_valid(monrace_id) && monraces.contains(monrace_id);
    };
    for (const auto &[monrace_id, monrace] : monraces) {
        const auto id = enum2i(monrace_id);
        if (monrace->next_r_idx != MonraceId::PLAYER && !is_defined(monrace->next_r_idx)) {
            return fmt::format(_("モンスターID {}の進化先 {} が定義されていません", "monster ID {}: evolution target {} is not defined"), id, enum2i(monrace->next_r_idx));
        }
        for (const auto &reinforce : monrace->get_reinforces()) {
            if (!is_defined(reinforce.get_monrace_id())) {
                return fmt::format(_("モンスターID {}の護衛 {} が定義されていません", "monster ID {}: escort {} is not defined"), id, enum2i(reinforce.get_monrace_id()));
            }
        }
        for (const auto &summon : monrace->get_final_summons()) {
            if (!is_defined(summon.id)) {
                return fmt::format(_("モンスターID {}の死亡時召喚 {} が定義されていません", "monster ID {}: final summon {} is not defined"), id, enum2i(summon.id));
            }
        }
        for (const auto &drop : monrace->get_drop_artifacts()) {
            if ((drop.fa_id == FixedArtifactId::NONE) || !artifacts.contains(drop.fa_id)) {
                return fmt::format(_("モンスターID {}の固定アーティファクト {} が定義されていません", "monster ID {}: fixed artifact {} is not defined"), id, enum2i(drop.fa_id));
            }
        }
    }

    return std::nullopt;
}
