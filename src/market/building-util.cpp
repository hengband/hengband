﻿#include "market/building-util.h"

#include "term/screen-processor.h"
/*!
 * @brief コンソールに表示された施設に関する情報を消去する / Clear the building information
 * @details 消去は行毎にヌル文字列で行われる。
 * @param min_row 開始行番号
 * @param max_row 末尾行番号
 * @return なし
 */
void clear_bldg(int min_row, int max_row)
{
	for (int i = min_row; i <= max_row; i++)
	{
		prt("", i, 0);
	}
}

/*!
 * @brief 所持金を表示する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void building_prt_gold(player_type *player_ptr)
{
    char tmp_str[80];
    prt(_("手持ちのお金: ", "Gold Remaining: "), 23, 53);
    sprintf(tmp_str, "%9ld", (long)player_ptr->au);
    prt(tmp_str, 23, 68);
}
