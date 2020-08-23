#include "angband.h"
#include "feature.h"


/*!
 * @brief 地形が罠持ちであるかの判定を行う。 / Return TRUE if the given feature is a trap
 * @param feat 地形情報のID
 * @return 罠持ちの地形ならばTRUEを返す。
 */
bool is_trap(FEAT_IDX feat)
{
	return have_flag(f_info[feat].flags, FF_TRAP);
}

/*!
 * @brief 地形が閉じたドアであるかの判定を行う。 / Return TRUE if the given grid is a closed door
 * @param feat 地形情報のID
 * @return 閉じたドアのある地形ならばTRUEを返す。
 */
bool is_closed_door(FEAT_IDX feat)
{
	feature_type *f_ptr = &f_info[feat];

	return (have_flag(f_ptr->flags, FF_OPEN) || have_flag(f_ptr->flags, FF_BASH)) &&
		!have_flag(f_ptr->flags, FF_MOVE);
}

