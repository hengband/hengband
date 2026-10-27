#include "info-reader/magic-reader.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/magic-info-tokens-table.h"
#include "info-reader/parse-error-types.h"
#include "locale/language-switcher.h"
#include "player-info/class-info.h"
#include "system/spell-info-list.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
//! 消費MP・初回経験値倍率・装備重量閾値の上限。
constexpr auto MAX_CLASS_MAGIC_VALUE = std::numeric_limits<short>::max() / 16;
}

MagicReader::Session MagicReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, static_cast<int>(class_magics_info.size()) - 1 }, _("職業魔法", "class magic"), last_id);
}

MagicReader::MagicReader(const nlohmann::json &class_data, Session &session)
    : class_data(class_data)
    , session(session)
{
}

/*!
 * @brief 職業魔法情報(ClassMagicDefinitions)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::read() const
{
    if (!this->class_data.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto class_id_result = this->read_class_id();
    if (!class_id_result) {
        return tl::make_unexpected(class_id_result.error());
    }

    const auto id_result = this->session.check(*class_id_result);
    if (!id_result) {
        msg_print(id_result.error().reason);
        return tl::unexpected(id_result.error().code);
    }
    const auto class_id = *id_result;
    // レコード全体が有効と確定するまで、既存の職業定義を維持する。
    player_magic magics_info = class_magics_info[class_id];

    if (const auto result = this->set_spell_type(magics_info); !result) {
        msg_print(_("呪文タイプ読込失敗。", "Failed to load spell type."));
        return result;
    }
    if (const auto result = this->set_magic_status(magics_info); !result) {
        msg_print(_("呪文行使に使用する能力値読込失敗。", "Failed to load magic status."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "has_glove_mp_penalty"), magics_info.has_glove_mp_penalty, true); !result) {
        msg_print(_("籠手によるMPペナルティ読込失敗。", "Failed to load glove_mp_penalty."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "has_magic_fail_rate_cap"), magics_info.has_magic_fail_rate_cap, true); !result) {
        msg_print(_("最低呪文失率情報読込失敗。", "Failed to load magic_fail_rate_cap."));
        return result;
    }
    if (const auto result = info_set_bool(get_json_value(this->class_data, "is_spell_trainable"), magics_info.is_spell_trainable, true); !result) {
        msg_print(_("呪文訓練可能性読込失敗。", "Failed to load spell_trainability."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->class_data, "first_spell_level"), magics_info.spell_first, true, Range(0, 99)); !result) {
        msg_print(_("呪文行使開始レベル読込失敗。", "Failed to load spell-first level."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->class_data, "armour_weight_limit"), magics_info.spell_weight, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("MP重量制限値読込失敗。", "Failed to load spell-weight value."));
        return result;
    }
    if (const auto result = this->set_realm_data(magics_info); !result) {
        msg_print(_("呪文データ読込失敗。", "Failed to load spell data."));
        return result;
    }

    class_magics_info[class_id] = std::move(magics_info);
    return {};
}

/*!
 * @brief JSON Objectから職業IDを取得する
 * @return 職業ID、失敗時はエラーコード
 */
tl::expected<int, ParseErrorType> MagicReader::read_class_id() const
{
    const auto &classname_obj = get_json_value(this->class_data, "name");
    if (!classname_obj.is_string()) {
        msg_print(_("職業名は文字列が必要です。", "Class name must be a string."));
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto player_class = magic_info_class_list.find(classname_obj.get<std::string>());
    if (player_class == magic_info_class_list.end()) {
        msg_print(_("未知の職業名 '{}'。", "Unknown class name '{}'."), classname_obj.get<std::string>());
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    return enum2i(player_class->second);
}

/*!
 * @brief JSON Objectから職業で使用する呪文タイプを取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_spell_type(player_magic &magics_info) const
{
    const auto &spell_book = get_json_value(this->class_data, "spell_type");
    if (!spell_book.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto tval = magic_info_spelltype_to_tval.find(spell_book.get<std::string>());
    if (tval == magic_info_spelltype_to_tval.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    magics_info.spell_book = tval->second;
    return {};
}

/*!
 * @brief JSON Objectから職業で呪文行使に使用する能力値を取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_magic_status(player_magic &magics_info) const
{
    const auto &magic_status = get_json_value(this->class_data, "magic_status");
    if (!magic_status.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    const auto status = magic_info_name_to_stat.find(magic_status.get<std::string>());
    if (status == magic_info_name_to_stat.end()) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    magics_info.spell_stat = status->second;
    return {};
}

/*!
 * @brief JSON Objectから各呪文の詳細を取得する
 * @param spell_data 各呪文情報の格納されたJSON Object
 * @param magics_info 保存先の魔法情報構造体
 * @param realm 呪文の領域
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_spell_data(const nlohmann::json &spell_data, player_magic &magics_info, RealmType realm) const
{
    const auto &spell_tag_obj = get_json_value(spell_data, "spell_tag");
    if (!spell_tag_obj.is_string()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    const auto spell_id = SpellInfoList::get_instance().get_spell_id(realm, spell_tag_obj.get<std::string>());
    if (!spell_id) {
        return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
    }
    auto &info = magics_info.info[enum2i(realm)][*spell_id];

    if (const auto result = info_set_integer(get_json_value(spell_data, "learn_level"), info.slevel, true, Range(0, 99)); !result) {
        msg_print(_("呪文学習レベル読込失敗。", "Failed to load spell learn_level."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "mana_cost"), info.smana, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("呪文コスト読込失敗。", "Failed to load spell mana_cost."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "difficulty"), info.sfail, true, Range(0, 100)); !result) {
        msg_print(_("呪文難易度読込失敗。", "Failed to load spell difficulty."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(spell_data, "first_cast_exp_rate"), info.sexp, true, Range(0, MAX_CLASS_MAGIC_VALUE)); !result) {
        msg_print(_("呪文詠唱ボーナスEXP読込失敗。", "Failed to load spell first_cast_exp_rate."));
        return result;
    }

    return {};
}

/*!
 * @brief JSON Objectから職業毎に定義された各呪文の詳細を取得する
 * @param magics_info 保存先の魔法情報構造体
 * @return 成功時は空の値、失敗時はエラーコード
 */
tl::expected<void, ParseErrorType> MagicReader::set_realm_data(player_magic &magics_info) const
{
    const auto &realms_data = get_json_value(this->class_data, "realms");
    if (realms_data.is_null()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!realms_data.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    for (const auto &realm : realms_data) {
        const auto &realm_name_obj = get_json_value(realm, "name");
        if (!realm_name_obj.is_string()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }

        const auto realm_name = realms_list.find(realm_name_obj.get<std::string>());
        if (realm_name == realms_list.end()) {
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }

        const auto realm_id = realm_name->second;
        const auto &spells_info_obj = get_json_value(realm, "spells_info");
        if (spells_info_obj.is_null()) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        if (!spells_info_obj.is_array()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }

        for (const auto &spell : spells_info_obj) {
            if (const auto result = this->set_spell_data(spell, magics_info, realm_id); !result) {
                return tl::make_unexpected(result.error());
            }
        }
    }

    return {};
}
