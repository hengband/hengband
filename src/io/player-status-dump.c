#include "angband.h"
#include "files.h"
#include "io/player-status-dump.h"

/*!
 * @brief プレイヤーのステータス表示をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
void dump_aux_player_status(player_type *creature_ptr, FILE *fff, display_player_pf display_player, map_name_pf map_name)
{
	TERM_COLOR a;
	char c;
	char buf[1024];
	display_player(creature_ptr, 0, map_name);

	for (TERM_LEN y = 1; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			buf[x] = c;
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, _("%s\n", "%s\n"), buf);
	}

	display_player(creature_ptr, 1, map_name);
	for (TERM_LEN y = 10; y < 19; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
			buf[x] = c;
		}

		buf[x] = '\0';
		while ((x > 0) && (buf[x - 1] == ' '))
			buf[--x] = '\0';

		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
	display_player(creature_ptr, 2, map_name);
	for (TERM_LEN y = 2; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
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

	fprintf(fff, "\n");
	display_player(creature_ptr, 3, map_name);
	for (TERM_LEN y = 1; y < 22; y++)
	{
		TERM_LEN x;
		for (x = 0; x < 79; x++)
		{
			(void)(Term_what(x, y, &a, &c));
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

	fprintf(fff, "\n");
}
