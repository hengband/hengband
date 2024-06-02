#include "info-reader/json-reader-util.h"
#include "info-reader/info-reader-util.h"
#include "locale/japanese.h"

/*!
 * @brief JSON Objectから文字列をセットする
 *
 * 引数で与えられたJSON Objectから日本語版の場合は"ja"、英語版の場合は"en"のキーで文字列を取得し、
 * data に格納する。キーが存在しない場合は is_required が真の場合はエラーを返し、偽の場合は何もせずに終了する。
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
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

#ifdef JP
    const auto &ja_str = json.find("ja");
    if (ja_str == json.end()) {
        return unexist_result;
    }
    auto ja_str_sys = utf8_to_sys(ja_str->get<std::string>());
    if (!ja_str_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    data = std::move(*ja_str_sys);
#else
    const auto &en_str = json.find("en");
    if (en_str == json.end()) {
        return unexist_result;
    }
    data = en_str->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからダイスの値を取得する
 * @param json ダイスの値が格納されたJSON Object
 * @param dd ダイスの数を格納する変数への参照
 * @param ds ダイスの面数を格納する変数への参照
 * @param is_required 必須かどうか
 * 必須でJSON Objectがnullや文字列でない場合はエラーを返す。必須でない場合は何もせずに終了する。
 * @return エラーコード
 */
errr info_set_dice(const nlohmann::json &json, DICE_NUMBER &dd, DICE_SID &ds, bool is_required)
{
    if (json.is_null() || !json.is_string()) {
        return is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;
    }

    return info_set_dice(json.get<std::string>(), dd, ds);
}
