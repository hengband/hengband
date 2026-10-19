#include "info-reader/json-reader-util.h"
#include "info-reader/info-reader-util.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "util/dice.h"

/*!
 * @brief 定義ファイルのルートとレコード配列を、入力を変更せず検証する。
 * @param root JSON文書のルート
 * @param key 必須のレコード配列キー
 * @param allow_empty 空配列を許容するか（Vaultではfalse）
 * @return 成功時は空の値、失敗時は解析エラーコード（ルートがオブジェクトでないか、キーの値が配列でなければ
 * PARSE_ERROR_INVALID_TYPE、キーが無ければPARSE_ERROR_TOO_FEW_ARGUMENTS、許容しない空配列ならPARSE_ERROR_INVALID_VALUE）
 */
tl::expected<void, parse_error_type> info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty)
{
    if (!root.is_object()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }
    const auto records = root.find(key);
    if (records == root.end()) {
        return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }
    if (!records->is_array()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }
    if (!allow_empty && records->empty()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_VALUE);
    }
    return {};
}

/*!
 * @brief オブジェクトのキーに対応する値への参照を返す。
 * オブジェクトでない入力やキー欠落は、例外ではなくstaticなnull値への参照を返す。
 * キーが存在する場合は入力内の値への参照なので、その寿命を超えて保持してはいけない。
 */
const nlohmann::json &get_json_value(const nlohmann::json &json, std::string_view key)
{
    static const nlohmann::json null_json;
    if (!json.is_object()) {
        return null_json;
    }

    const auto it = json.find(key);
    return it != json.end() ? *it : null_json;
}

/*!
 * @brief JSON Objectから文字列をセットする
 *
 * 引数で与えられたJSON Objectから日本語版の場合は"ja"、英語版の場合は"en"のキーで文字列を取得し、
 * data に格納する。キーが存在しない場合は is_required が真の場合はエラーを返し、偽の場合は何もせずに終了する。
 * nullでない非オブジェクトや選択言語の非文字列値は、必須かどうかによらずPARSE_ERROR_INVALID_TYPEを返す。
 * 選択されない言語の値は検証しない。日本語版は既存の文字コード変換に従い、変換失敗ならPARSE_ERROR_INVALID_FLAGを返す。
 * エラーまたは任意の欠落の場合、dataは変更しない。
 *
 * @param json 文字列の格納されたJSON Object
 * @param data 文字列を格納する変数への参照
 * @param is_required 必須かどうか
 * @return 成功時は空の値、失敗時は解析エラーコード（必須の文字列が無ければPARSE_ERROR_TOO_FEW_ARGUMENTS、
 * 型が誤っていればPARSE_ERROR_INVALID_TYPE、日本語版で文字コードの変換に失敗すればPARSE_ERROR_INVALID_FLAG）
 */
tl::expected<void, parse_error_type> info_set_string(const nlohmann::json &json, std::string &data, bool is_required)
{
    const auto unexist_result = [is_required]() -> tl::expected<void, parse_error_type> {
        if (is_required) {
            return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
        return {};
    };

    if (json.is_null()) {
        return unexist_result();
    }

    if (!json.is_object()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }

    const auto str = json.find(_("ja", "en"));
    if (str == json.end()) {
        return unexist_result();
    }
    if (!str->is_string()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }

#ifdef JP
    auto str_sys = utf8_to_sys(str->get<std::string>());
    if (!str_sys) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_FLAG);
    }
    data = std::move(*str_sys);
#else
    data = str->get<std::string>();
#endif

    return {};
}

/*!
 * @brief JSON Objectからダイスの値を取得する
 * @param json ダイスの値が格納されたJSON Object
 * @param dice ダイスの値を格納する変数への参照
 * @param is_required 必須かどうか
 * @param range ダイスの個数と面数それぞれの範囲（両端を含む）
 * @return 成功時は空の値、失敗時は解析エラーコード（PARSE_ERROR_TOO_FEW_ARGUMENTS、PARSE_ERROR_INVALID_TYPE、
 * PARSE_ERROR_INVALID_FLAGのいずれか。条件は下記のとおり）
 * @details
 * JSON値がnullの場合、必須ならPARSE_ERROR_TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非文字列値は、必須かどうかによらずPARSE_ERROR_INVALID_TYPEを返す。
 * Dice::parseがruntime_errorを投げる文字列はPARSE_ERROR_TOO_FEW_ARGUMENTSを返す。
 * 個数か面数がrangeの範囲外ならPARSE_ERROR_INVALID_FLAGを返す。
 * エラーまたは任意のnullの場合、diceは変更しない。
 */
tl::expected<void, parse_error_type> info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required, tl::optional<Range> range)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
        return {};
    }
    if (!json.is_string()) {
        return tl::make_unexpected(PARSE_ERROR_INVALID_TYPE);
    }

    try {
        const auto parsed = Dice::parse(json.get<std::string>());
        const auto is_in_range = [&range](int value) { return !range || ((range->first <= value) && (value <= range->second)); };
        if (!is_in_range(parsed.num) || !is_in_range(parsed.sides)) {
            return tl::make_unexpected(PARSE_ERROR_INVALID_FLAG);
        }
        dice = parsed;
        return {};
    } catch (const std::runtime_error &) {
        return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }
}

/*!
 * @brief JSON Objectからbool値を取得する
 * @param json bool値が格納されたJSON Object
 * @param bool_value bool値を格納する変数への参照
 * @param is_required 必須かどうか
 * nullおよび非bool値は、必須ならPARSE_ERROR_TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * 型不一致でもPARSE_ERROR_INVALID_TYPEにはしない、他のinfo_set_*とは異なる契約を持つ。
 * bool値が入力された場合のみbool_valueを変更する。
 * @return 成功時は空の値、失敗時は解析エラーコード（必須の値がnullか非bool値ならPARSE_ERROR_TOO_FEW_ARGUMENTS）
 */
tl::expected<void, parse_error_type> info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required)
{
    if (json.is_null() || !json.is_boolean()) {
        if (is_required) {
            return tl::make_unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
        }
        return {};
    }

    bool_value = json.get<bool>();
    return {};
}
