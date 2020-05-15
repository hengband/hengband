#include "system/angband.h"
#include "market/building-util.h"

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
