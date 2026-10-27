#include "info-reader/info-reader-util.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/activation-info-table.h"
#include "view/display-messages.h"

/* Help give useful error messages */

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
