#include "io-dump/player-status-dump.h"

/*!
 * @brief 画面番号を指定してダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @param display_player 画面表示へのコールバック
 * @param screen_num 画面番号
 * @param start_y
 * @param end_y
 * @param change_color バッファへ詰める文字の変更有無
 * @return なし
 */
static void dump_player_status_with_screen_num(
	player_type *creature_ptr, FILE *fff, display_player_pf display_player,
	int screen_num, TERM_LEN start_y, TERM_LEN end_y, bool change_color)
{
	TERM_COLOR a;
	char c;
	char buf[1024];
	display_player(creature_ptr, screen_num);
	for (TERM_LEN y = start_y; y < end_y; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(term_what(x, y, &a, &c));
			if (!change_color)
			{
				buf[x] = c;
				continue;
			}

			if (a < 128)
				buf[x] = c;
			else
				buf[x] = ' ';
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, "%s\n", buf);
	}
}


/*!
 * @brief プレイヤーのステータス表示をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @param display_player 画面表示へのコールバック
 * @return なし
 */
void dump_aux_player_status(player_type *creature_ptr, FILE *fff, display_player_pf display_player)
{
	dump_player_status_with_screen_num(creature_ptr, fff, display_player, 0, 1, 22, FALSE);
	dump_player_status_with_screen_num(creature_ptr, fff, display_player, 1, 10, 19, FALSE);
	fprintf(fff, "\n");
	dump_player_status_with_screen_num(creature_ptr, fff, display_player, 2, 2, 22, TRUE);
	fprintf(fff, "\n");
	dump_player_status_with_screen_num(creature_ptr, fff, display_player, 3, 1, 22, TRUE);
	fprintf(fff, "\n");
}
