#include "angband.h"
#include "object-boost.h"

/*!
 * @brief 上質以上のオブジェクトに与えるための各種ボーナスを正規乱数も加えて算出する。
 * Help determine an "enchantment bonus" for an object.
 * @param max ボーナス値の限度
 * @param level ボーナス値に加味する基準生成階
 * @return 算出されたボーナス値
 * @details
 * To avoid floating point but still provide a smooth distribution of bonuses,\n
 * we simply round the results of division in such a way as to "average" the\n
 * correct floating point value.\n
 *\n
 * This function has been changed.  It uses "randnor()" to choose values from\n
 * a normal distribution, whose mean moves from zero towards the max as the\n
 * level increases, and whose standard deviation is equal to 1/4 of the max,\n
 * and whose values are forced to lie between zero and the max, inclusive.\n
 *\n
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very\n
 * rare to get the "full" enchantment on an object, even a deep levels.\n
 *\n
 * It is always possible (albeit unlikely) to get the "full" enchantment.\n
 *\n
 * A sample distribution of values from "m_bonus(10, N)" is shown below:\n
 *\n
 *   N       0     1     2     3     4     5     6     7     8     9    10\n
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----\n
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03\n
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05\n
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05\n
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11\n
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41\n
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65\n
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94\n
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78\n
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64\n
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62\n
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33\n
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38\n
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53\n
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53\n
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27\n
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72\n
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07\n
 */
int m_bonus(int max, DEPTH level)
{
	int bonus, stand, extra, value;


	/* Paranoia -- enforce maximal "level" */
	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;


	/* The "bonus" moves towards the max */
	bonus = ((max * level) / MAX_DEPTH);

	/* Hack -- determine fraction of error */
	extra = ((max * level) % MAX_DEPTH);

	/* Hack -- simulate floating point computations */
	if (randint0(MAX_DEPTH) < extra) bonus++;


	/* The "stand" is equal to one quarter of the max */
	stand = (max / 4);

	/* Hack -- determine fraction of error */
	extra = (max % 4);

	/* Hack -- simulate floating point computations */
	if (randint0(4) < extra) stand++;


	/* Choose an "interesting" value */
	value = randnor(bonus, stand);

	/* Enforce the minimum value */
	if (value < 0) return (0);

	/* Enforce the maximum value */
	if (value > max) return (max);
	return (value);
}

/*!
 * @brief 対象のオブジェクトにランダムな能力維持を一つ付加する。/ Choose one random sustain
 * @details 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_sustain(object_type *o_ptr)
{
	switch (randint0(A_MAX))
	{
	case 0: add_flag(o_ptr->art_flags, TR_SUST_STR); break;
	case 1: add_flag(o_ptr->art_flags, TR_SUST_INT); break;
	case 2: add_flag(o_ptr->art_flags, TR_SUST_WIS); break;
	case 3: add_flag(o_ptr->art_flags, TR_SUST_DEX); break;
	case 4: add_flag(o_ptr->art_flags, TR_SUST_CON); break;
	case 5: add_flag(o_ptr->art_flags, TR_SUST_CHR); break;
	}
}
