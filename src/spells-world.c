#include "angband.h"

/*!
 * @brief プレイ日数を変更する / Set gametime.
 * @return 実際に変更を行ったらTRUEを返す
 */
bool set_gametime(void)
{
	int tmp_int = 0;
	char ppp[80], tmp_val[40];

	sprintf(ppp, "Dungeon Turn (0-%ld): ", (long)dungeon_turn_limit);
	sprintf(tmp_val, "%ld", (long)dungeon_turn);
	if (!get_string(ppp, tmp_val, 10)) return (FALSE);
	tmp_int = atoi(tmp_val);

	/* Verify */
	if (tmp_int >= dungeon_turn_limit) tmp_int = dungeon_turn_limit - 1;
	else if (tmp_int < 0) tmp_int = 0;
	dungeon_turn = turn = tmp_int;
	return (TRUE);

}

