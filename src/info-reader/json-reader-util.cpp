#include "info-reader/json-reader-util.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/symbol-reader-util.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "term/gameterm.h"
#include "util/dice.h"

/*!
 * @brief フラグの文字列配列を先頭から読み込み、各トークンを呼び出し元へ渡す。
 * @details nullや欠落、空配列の可否は呼び出し元で判断する。重複を除去せず、
 * 最初のエラーで停止する。エラーより前に処理済みのフラグは巻き戻さない。
 * @param json フラグ配列
 * @param grab_flag 各文字列の解釈と格納を行う関数。未知のトークンならfalseを返す。
 * @return 成功時は空、型の誤りはINVALID_TYPE、未知のトークンはINVALID_FLAG
 */
tl::expected<void, ParseErrorType> info_read_flag_array(const nlohmann::json &json, const std::function<bool(const nlohmann::json &)> &grab_flag)
{
    if (!json.is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    for (const auto &flag : json) {
        if (!flag.is_string()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        if (!grab_flag(flag)) {
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }
    }
    return {};
}

/*!
 * @brief 定義ファイルのルートとレコード配列を、入力を変更せず検証する。
 * @param root JSON文書のルート
 * @param key 必須のレコード配列キー
 * @param allow_empty 空配列を許容するか（Vaultではfalse）
 * @return 成功時は空の値、失敗時は解析エラーコード（ルートがオブジェクトでないか、キーの値が配列でなければ
 * ParseErrorType::INVALID_TYPE、キーが無ければParseErrorType::TOO_FEW_ARGUMENTS、許容しない空配列ならParseErrorType::INVALID_VALUE）
 */
tl::expected<void, ParseErrorType> info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty)
{
    if (!root.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    const auto records = root.find(key);
    if (records == root.end()) {
        return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!records->is_array()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    if (!allow_empty && records->empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
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
 * nullでない非オブジェクトや選択言語の非文字列値は、必須かどうかによらずParseErrorType::INVALID_TYPEを返す。
 * 選択されない言語の値は検証しない。日本語版は既存の文字コード変換に従い、変換失敗ならParseErrorType::INVALID_VALUEを返す。
 * エラーまたは任意の欠落の場合、dataは変更しない。
 *
 * @param json 文字列の格納されたJSON Object
 * @param data 文字列を格納する変数への参照
 * @param is_required 必須かどうか
 * @return 成功時は空の値、失敗時は解析エラーコード（必須の文字列が無ければParseErrorType::TOO_FEW_ARGUMENTS、
 * 型が誤っていればParseErrorType::INVALID_TYPE、日本語版で文字コードの変換に失敗すればParseErrorType::INVALID_VALUE）
 */
tl::expected<void, ParseErrorType> info_set_string(const nlohmann::json &json, std::string &data, bool is_required)
{
    const auto unexist_result = [is_required]() -> tl::expected<void, ParseErrorType> {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    };

    if (json.is_null()) {
        return unexist_result();
    }

    if (!json.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto str = json.find(_("ja", "en"));
    if (str == json.end()) {
        return unexist_result();
    }
    if (!str->is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

#ifdef JP
    auto str_sys = utf8_to_sys(str->get<std::string>());
    if (!str_sys) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
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
 * @return 成功時は空の値、失敗時は解析エラーコード（ParseErrorType::TOO_FEW_ARGUMENTS、ParseErrorType::INVALID_TYPE、
 * ParseErrorType::INVALID_VALUEのいずれか。条件は下記のとおり）
 * @details
 * JSON値がnullの場合、必須ならParseErrorType::TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非文字列値は、必須かどうかによらずParseErrorType::INVALID_TYPEを返す。
 * Dice::parseがruntime_errorを投げる文字列と、個数か面数がrangeの範囲外のダイスはParseErrorType::INVALID_VALUEを返す。
 * エラーまたは任意のnullの場合、diceは変更しない。
 */
tl::expected<void, ParseErrorType> info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required, tl::optional<Range> range)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }
    if (!json.is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    try {
        const auto parsed = Dice::parse(json.get<std::string>());
        const auto is_in_range = [&range](int value) { return !range || ((range->first <= value) && (value <= range->second)); };
        if (!is_in_range(parsed.num) || !is_in_range(parsed.sides)) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        dice = parsed;
        return {};
    } catch (const std::runtime_error &) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
}

/*!
 * @brief JSON Objectからbool値を取得する
 * @param json bool値が格納されたJSON Object
 * @param bool_value bool値を格納する変数への参照
 * @param is_required 必須かどうか
 * nullは、必須ならParseErrorType::TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非bool値はParseErrorType::INVALID_TYPEを返す。
 * bool値が入力された場合のみbool_valueを変更する。
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }

    if (!json.is_boolean()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    bool_value = json.get<bool>();
    return {};
}

/*!
 * @brief 表示記号の文字と色を読み取り、既存リーダーの検証順序を保つ。
 * @param json character と color を持つオブジェクト
 * @param policy 通常は型・文字数・色の順。モンスターは型不一致も欠落として扱い、色を文字数より先に検証する。
 * @return 表示記号、または欠落・型・文字数・未知の色に対応する解析エラー
 * @details 文字は1バイトなら制御文字や非ASCIIも許容する。地形固有の lit とメッセージは呼出側で扱う。
 */
tl::expected<DisplaySymbol, ParseErrorType> info_read_symbol(const nlohmann::json &json, SymbolReadPolicy policy)
{
    const auto monster = policy == SymbolReadPolicy::MONSTER_COLOR_FIRST;
    if (json.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }
    if (!json.is_object()) {
        return tl::unexpected(monster ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }

    const auto &character_obj = get_json_value(json, "character");
    if (!character_obj.is_string()) {
        return tl::unexpected(monster || character_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    const auto &character = character_obj.get_ref<const std::string &>();
    if (!monster && character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    const auto &color_obj = get_json_value(json, "color");
    if (!color_obj.is_string()) {
        return tl::unexpected(monster || color_obj.is_null() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    const auto color = color_list.find(color_obj.get<std::string>());
    if (color == color_list.end()) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }
    if (character.size() != 1) {
        return tl::unexpected(ParseErrorType::INVALID_VALUE);
    }

    return DisplaySymbol(color->second, character.front());
}

/*!
 * @brief JSON文書の整数バージョンを検証する。
 * @param root JSON文書のルート
 * @param supported 対応するバージョン
 * @return 欠落・nullはTOO_FEW_ARGUMENTS、ルートや値の型の誤りはINVALID_TYPE、未対応の値はINVALID_VALUE。
 */
tl::expected<void, ParseErrorType> info_validate_version(const nlohmann::json &root, int supported)
{
    if (!root.is_object()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    auto version = 0;
    return info_set_integer(get_json_value(root, "version"), version, true, Range(supported, supported));
}
