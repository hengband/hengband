#include "mimic-info-table.h"

#ifdef JP
#define N(JAPANESE, ENGLISH) JAPANESE, ENGLISH
#else
#define N(JAPANESE, ENGLISH) ENGLISH
#endif

/*!
 * @brief 変身種族情報
 */
const player_race mimic_info[MAX_MIMIC_FORMS] =
{
	{
        N("[標準形態]", "Default"), "N",
		{  0,  0,  0,  0,  0,  0 },
		0,  0,  0,  0,  0,  10,  0,  0,
		10,  100,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		0,
		0x000000,
	},
	{
		N("[悪魔]", "[Demon]"), "uU",
		{  5,  3,  2,  3,  4,  -6 },
		-5,  18, 20, -2,  3,  10, 40, 20,
		12,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		5,
		0x000003,
	},
	{
		N("[魔王]", "[Demon lord]"), "U",
		{  20,  20,  20,  20,  20,  20 },
		20,  20, 25, -2,  3,  10, 70, 40,
		14,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		20,
		0x000003,
	},
	{
		N("[吸血鬼]", "[Vampire]"), "V",
		{ 4, 4, 1, 1, 2, 3 },
		6, 12, 8, 6, 2, 12, 30, 20,
		11,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		5,
		0x000005,
	},
};

