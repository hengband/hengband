#include "angband.h"
#include "player-item.h"

/*!
 * @brief get_item()からのリファクタリング
 * @return アイテムのポインタ、キャンセルや不能ならNULLを返す。
 */
object_type *ref_item(player_type *player_ptr, cptr pmt, cptr str, BIT_FLAGS mode)
{
	OBJECT_IDX idx;
	(player_ptr); // しばらくは未使用

	if(!get_item(&idx, pmt, str, mode)) return NULL;
	return idx >= 0 ? &inventory[idx] : &o_list[0 - idx];
}