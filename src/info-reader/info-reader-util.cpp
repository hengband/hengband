#include "info-reader/info-reader-util.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/activation-info-table.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

/* Help give useful error messages */
int error_idx; /*!< データ読み込み/初期化時に汎用的にエラーコードを保存するグローバル変数 */

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @details 数値指定も発動効果テーブルに登録されたIDだけを受け入れる。
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

    const auto j = std::stoi(what.data());
    if ((j > 0) && (j < enum2i(RandomArtActType::MAX))) {
        for (const auto &activation : activation_info) {
            if (j == enum2i(activation.index)) {
                return activation.index;
            }
        }
    }

    msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what.data());
    return RandomArtActType::NONE;
}
