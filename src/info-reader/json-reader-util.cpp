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
 * @return エラーコード
 */
errr info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty)
{
    if (!root.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    const auto records = root.find(key);
    if (records == root.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!records->is_array()) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    if (!allow_empty && records->empty()) {
        return PARSE_ERROR_INVALID_VALUE;
    }
    return PARSE_ERROR_NONE;
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
 * @return エラーコード
 */
errr info_set_string(const nlohmann::json &json, std::string &data, bool is_required)
{
    const auto unexist_result = is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;

    if (json.is_null()) {
        return unexist_result;
    }

    if (!json.is_object()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto str = json.find(_("ja", "en"));
    if (str == json.end()) {
        return unexist_result;
    }
    if (!str->is_string()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

#ifdef JP
    auto str_sys = utf8_to_sys(str->get<std::string>());
    if (!str_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    data = std::move(*str_sys);
#else
    data = str->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからダイスの値を取得する
 * @param json ダイスの値が格納されたJSON Object
 * @param dice ダイスの値を格納する変数への参照
 * @param is_required 必須かどうか
 * JSON値がnullの場合、必須ならPARSE_ERROR_TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非文字列値は、必須かどうかによらずPARSE_ERROR_INVALID_TYPEを返す。
 * Dice::parseがruntime_errorを投げる文字列はPARSE_ERROR_TOO_FEW_ARGUMENTSを返す。
 * エラーまたは任意のnullの場合、diceは変更しない。
 * @return エラーコード
 */
errr info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required)
{
    if (json.is_null()) {
        return is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;
    }
    if (!json.is_string()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    try {
        dice = Dice::parse(json.get<std::string>());
        return PARSE_ERROR_NONE;
    } catch (const std::runtime_error &) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
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
 * @return エラーコード
 */
errr info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required)
{
    if (json.is_null() || !json.is_boolean()) {
        return is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;
    }

    bool_value = json.get<bool>();
    return PARSE_ERROR_NONE;
}
