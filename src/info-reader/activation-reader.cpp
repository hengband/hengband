#include "info-reader/activation-reader.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/activation-info-table.h"
#include "view/display-messages.h"
#include <nlohmann/json.hpp>

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @details 発動効果テーブルに登録された名前だけを受け入れる。番号は効果を足すとずれるため、番号での指定は受け付けない。
 * @param what 発動の名前
 * @return 発動能力ID
 */
RandomArtActType grab_one_activation_flag(std::string_view what)
{
    for (const auto &activation : activation_info) {
        if (what == activation.flag) {
            return activation.index;
        }
    }

    msg_print(_("未知の発動・フラグ '{}'。", "Unknown activation flag '{}'."), what);
    return RandomArtActType::NONE;
}

/*!
 * @brief 任意の発動指定を読み込み、未知の名前の理由を表示する
 * @param activation_json 発動名の JSON 値。省略された項目は null として渡す
 * @return 省略時は NONE、指定時は発動能力 ID、失敗時は解析エラーコード
 */
tl::expected<RandomArtActType, ParseErrorType> read_activation(const nlohmann::json &activation_json)
{
    if (activation_json.is_null()) {
        return RandomArtActType::NONE;
    }
    if (!activation_json.is_string()) {
        return tl::unexpected(ParseErrorType::INVALID_TYPE);
    }
    const auto activation = grab_one_activation_flag(activation_json.get<std::string>());
    if (activation == RandomArtActType::NONE) {
        return tl::unexpected(ParseErrorType::INVALID_FLAG);
    }
    return activation;
}
