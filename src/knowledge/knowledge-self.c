/*!
 * @brief 自己に関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "angband.h"
#include "knowledge-self.h"
#include "cmd/dump-util.h"
#include "avatar.h"
#include "birth/birth.h"
#include "core/show-file.h"
#include "dungeon/dungeon-file.h"
#include "world/world.h"
#include "market/store-util.h"
#include "floor/floor-town.h"
#include "object-flavor.h"

/*
 * List virtues & status
 */
void do_cmd_knowledge_virtues(player_type *creature_ptr)
{
	FILE *fff = NULL;
	GAME_TEXT file_name[FILE_NAME_SIZE];
	if (!open_temporary_file(&fff, file_name)) return;

	fprintf(fff, _("現在の属性 : %s\n\n", "Your alignment : %s\n\n"), your_alignment(creature_ptr));
	dump_virtues(creature_ptr, fff);
	my_fclose(fff);
	(void)show_file(creature_ptr, TRUE, file_name, _("八つの徳", "Virtues"), 0, 0);
	fd_kill(file_name);
}


/*
* List virtues & status
*
*/
void do_cmd_knowledge_stat(player_type *creature_ptr)
{
	FILE *fff = NULL;
	GAME_TEXT file_name[FILE_NAME_SIZE];
	if (!open_temporary_file(&fff, file_name)) return;

	int percent = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
		(2 * creature_ptr->hitdie +
		((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

	if (creature_ptr->knowledge & KNOW_HPRATE)
		fprintf(fff, _("現在の体力ランク : %d/100\n\n", "Your current Life Rating is %d/100.\n\n"), percent);
	else fprintf(fff, _("現在の体力ランク : ???\n\n", "Your current Life Rating is ???.\n\n"));

	fprintf(fff, _("能力の最大値\n\n", "Limits of maximum stats\n\n"));
	for (int v_nr = 0; v_nr < A_MAX; v_nr++)
	{
		if ((creature_ptr->knowledge & KNOW_STAT) || creature_ptr->stat_max[v_nr] == creature_ptr->stat_max_max[v_nr])
			fprintf(fff, "%s 18/%d\n", stat_names[v_nr], creature_ptr->stat_max_max[v_nr] - 18);
		else
			fprintf(fff, "%s ???\n", stat_names[v_nr]);
	}

	dump_yourself(creature_ptr, fff);
	my_fclose(fff);
	(void)show_file(creature_ptr, TRUE, file_name, _("自分に関する情報", "HP-rate & Max stat"), 0, 0);
	fd_kill(file_name);
}


/*
 * List my home
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_home(player_type *player_ptr)
{
	process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);

	FILE *fff = NULL;
	GAME_TEXT file_name[FILE_NAME_SIZE];
	if (!open_temporary_file(&fff, file_name)) return;

	store_type *store_ptr;
	store_ptr = &town_info[1].store[STORE_HOME];

	if (store_ptr->stock_num)
	{
#ifdef JP
		TERM_LEN x = 1;
#endif
		fprintf(fff, _("  [ 我が家のアイテム ]\n", "  [Home Inventory]\n"));
		concptr	paren = ")";
		GAME_TEXT o_name[MAX_NLEN];
		for (int i = 0; i < store_ptr->stock_num; i++)
		{
#ifdef JP
			if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
			object_desc(player_ptr, o_name, &store_ptr->stock[i], 0);
			if (strlen(o_name) <= 80 - 3)
			{
				fprintf(fff, "%c%s %s\n", I2A(i % 12), paren, o_name);
			}
			else
			{
				int n;
				char *t;
				for (n = 0, t = o_name; n < 80 - 3; n++, t++)
					if (iskanji(*t)) { t++; n++; }
				if (n == 81 - 3) n = 79 - 3; /* 最後が漢字半分 */

				fprintf(fff, "%c%s %.*s\n", I2A(i % 12), paren, n, o_name);
				fprintf(fff, "   %.77s\n", o_name + n);
			}
#else
			object_desc(player_ptr, o_name, &store_ptr->stock[i], 0);
			fprintf(fff, "%c%s %s\n", I2A(i % 12), paren, o_name);
#endif
		}

		fprintf(fff, "\n\n");
	}

	my_fclose(fff);
	(void)show_file(player_ptr, TRUE, file_name, _("我が家のアイテム", "Home Inventory"), 0, 0);
	fd_kill(file_name);
}
