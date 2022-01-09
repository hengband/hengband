#include "io-dump/player-status-dump.h"
#include "view/display-player.h"

/*!
 * @brief 画面番号を指定してダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @param display_player 画面表示へのコールバック
 * @param mode 表示モード
 * @param start_y ダンプの開始行数
 * @param end_y ダンプの終了行数
 * @param change_color バッファへ詰める文字の変更有無
 */
static void dump_player_status_with_screen_num(PlayerType *player_ptr, FILE *fff, int mode, TERM_LEN start_y, TERM_LEN end_y, bool change_color)
{
	TERM_COLOR a;
	char c;
	char buf[1024];
	display_player(player_ptr, mode);
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_aux_player_status(PlayerType *player_ptr, FILE *fff)
{
	dump_player_status_with_screen_num(player_ptr, fff, 0, 1, 22, false);
	dump_player_status_with_screen_num(player_ptr, fff, 1, 10, 19, false);
	fprintf(fff, "\n");
	dump_player_status_with_screen_num(player_ptr, fff, 2, 2, 22, true);
	fprintf(fff, "\n");
	dump_player_status_with_screen_num(player_ptr, fff, 3, 1, 18, true);
	fprintf(fff, "\n");
    dump_player_status_with_screen_num(player_ptr, fff, 4, 1, 19, true);
    fprintf(fff, "\n");
}
