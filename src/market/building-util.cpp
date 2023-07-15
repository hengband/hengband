#include "market/building-util.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"

/*!
 * @brief コンソールに表示された施設に関する情報を消去する / Clear the building information
 * @details 消去は行毎にヌル文字列で行われる。
 * @param min_row 開始行番号
 * @param max_row 末尾行番号
 */
void clear_bldg(int min_row, int max_row)
{
    for (int i = min_row; i <= max_row; i++) {
        prt("", i, 0);
    }
}

/*!
 * @brief 所持金を表示する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void building_prt_gold(PlayerType *player_ptr)
{
    prt(_("手持ちのお金: ", "Gold Remaining: "), 23, 53);
    prt(format("%9ld", (long)player_ptr->au), 23, 68);
}
