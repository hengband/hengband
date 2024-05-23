#include "info-reader/info-reader-util.h"
#include "artifact/random-art-effects.h"
#include "main/angband-headers.h"
#include "object-enchant/activation-info-table.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/* Help give useful error messages */
int error_idx; /*!< データ読み込み/初期化時に汎用的にエラーコードを保存するグローバル変数 */
int error_line; /*!< データ読み込み/初期化時に汎用的にエラー行数を保存するグローバル変数 */

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @param what 参照元の文字列ポインタ
 * @return 発動能力ID
 */
RandomArtActType grab_one_activation_flag(std::string_view what)
{
    for (const auto &activation : activation_info) {
        if (what == activation.flag) {
            return activation.index;
        }
    }

    auto j = std::stoi(what.data());
    if (j > 0) {
        return i2enum<RandomArtActType>(j);
    }

    msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what.data());
    return RandomArtActType::NONE;
}

#ifndef JP
/*!
 * @brief 英語のフレーバーテキストを連結する
 * @details add は両端のスペースを削除し、text が空でなく連結する場合は text の末尾が
 * ".!?" のいずれかならスペースを2つ、それ以外ならスペースを1つ挟んで連結する。
 *
 * @param text 現在までに作成されたフレーバーテキスト
 * @param add 連結するテキスト
 */
void append_english_text(std::string &text, std::string_view add)
{
    const auto add_trimmed = str_trim(add);
    if (add_trimmed.empty()) {
        return;
    }

    if (!text.empty()) {
        constexpr std::string_view eos_symbols = ".!?";
        auto is_eos = eos_symbols.find(text.back()) != std::string_view::npos;
        text.append(is_eos ? "  " : " ");
    }
    text.append(add_trimmed);
}
#endif

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
 * @brief ダイスを表す文字列を解析してダイスの値をセットする
 *
 * 引数で与えられた文字列 "XdY" をダイスの値に変換し、X を dd に、Y を ds に格納する。
 * 文字列が正しくダイスとして解釈できない場合はエラーを返す。
 *
 * @param dice_str ダイスを表す文字列
 * @param dd ダイスの数を格納する変数への参照
 * @param ds ダイスの面数を格納する変数への参照
 * @return エラーコード
 */
errr info_set_dice(const std::string_view dice_str, DICE_NUMBER &dd, DICE_SID &ds)
{
    const auto &dice = str_split(dice_str, 'd', false, 2);
    if (dice.size() < 2) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    dd = std::stoi(dice[0]);
    ds = std::stoi(dice[1]);

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
