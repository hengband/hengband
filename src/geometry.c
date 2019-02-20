#include "angband.h"

/*!
 * @brief プレイヤーから指定の座標がどの方角にあるかを返す /
 * Convert an adjacent location to a direction.
 * @param y 方角を確認したY座標
 * @param x 方角を確認したX座標
 * @return 方向ID
 */
DIRECTION coords_to_dir(POSITION y, POSITION x)
{
	DIRECTION d[3][3] = { {7, 4, 1}, {8, 5, 2}, {9, 6, 3} };
	POSITION dy, dx;

	dy = y - p_ptr->y;
	dx = x - p_ptr->x;

	/* Paranoia */
	if (ABS(dx) > 1 || ABS(dy) > 1) return (0);

	return d[dx + 1][dy + 1];
}
