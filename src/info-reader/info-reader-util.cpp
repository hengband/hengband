#include "info-reader/info-reader-util.h"
#include "artifact/random-art-effects.h"
#include "main/angband-headers.h"
#include "object-enchant/activation-info-table.h"
#include "view/display-messages.h"
#include "util/enum-converter.h"

/* Help give useful error messages */
int error_idx; /*!< データ読み込み/初期化時に汎用的にエラーコードを保存するグローバル変数 */
int error_line; /*!< データ読み込み/初期化時に汎用的にエラー行数を保存するグローバル変数 */

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(発動能力用) /
 * Grab one activation index flag
 * @param what 参照元の文字列ポインタ
 * @return 発動能力ID
 */
RandomArtActType grab_one_activation_flag(concptr what)
{
    for (auto i = 0;; i++) {
        if (activation_info[i].flag == nullptr) {
            break;
        }

        if (streq(what, activation_info[i].flag)) {
            return activation_info[i].index;
        }
    }

    auto j = atoi(what);
    if (j > 0) {
        return i2enum<RandomArtActType>(j);
    }

    msg_format(_("未知の発動・フラグ '%s'。", "Unknown activation flag '%s'."), what);
    return RandomArtActType::NONE;
}
