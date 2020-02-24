/*!
 * @brief 死亡・引退・切腹時の画面表示
 * @date 2020/02/24
 * @author Hourier
 * @details
 * core、files、view-mainwindowの参照禁止。コールバックで対応すること
 */

#include "process-death.h"
#include "world.h"
#include "floor-town.h"
#include "player-inventory.h"
#include "object-flavor.h"
#include "store.h"
#include "term.h"

#define GRAVE_LINE_WIDTH 31

concptr ANGBAND_DIR_FILE; //!< Various extra files (ascii) These files may be portable between platforms

/*!
 * @brief 墓石の真ん中に文字列を書き込む /
 * Centers a string within a GRAVE_LINE_WIDTH character string		-JWT-
 * @return なし
 * @details
 */
static void center_string(char *buf, concptr str)
{
	int i = strlen(str);
	int j = GRAVE_LINE_WIDTH / 2 - i / 2;
	(void)sprintf(buf, "%*s%s%*s", j, "", str, GRAVE_LINE_WIDTH - i - j, "");
}


/*
 * Redefinable "print_tombstone" action
 */
bool(*tombstone_aux)(void) = NULL;


/*!
 * @brief 墓石のアスキーアート表示 /
 * Display a "tomb-stone"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void print_tomb(player_type *dead_ptr)
{
	bool done = FALSE;
	if (tombstone_aux)
	{
		done = (*tombstone_aux)();
	}

	if (done) return;

#ifdef JP
	int extra_line = 0;
#endif
	Term_clear();
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("dead_j.txt", "dead.txt"));

	FILE *fp;
	fp = my_fopen(buf, "r");

	if (fp)
	{
		int i = 0;

		/* Dump the file to the screen */
		while (my_fgets(fp, buf, sizeof(buf)) == 0)
		{
			put_str(buf, i++, 0);
		}

		my_fclose(fp);
	}

	concptr p;
	if (current_world_ptr->total_winner || (dead_ptr->lev > PY_MAX_LEVEL))
	{
#ifdef JP
		p = "偉大なる者";
#else
		p = "Magnificent";
#endif
	}
	else
	{
		p = player_title[dead_ptr->pclass][(dead_ptr->lev - 1) / 5];
	}

	center_string(buf, dead_ptr->name);
	put_str(buf, 6, 11);

#ifdef JP
#else
	center_string(buf, "the");
	put_str(buf, 7, 11);
#endif

	center_string(buf, p);
	put_str(buf, 8, 11);

	center_string(buf, cp_ptr->title);
	put_str(buf, 10, 11);

	char tmp[160];
	(void)sprintf(tmp, _("レベル: %d", "Level: %d"), (int)dead_ptr->lev);
	center_string(buf, tmp);
	put_str(buf, 11, 11);

	(void)sprintf(tmp, _("経験値: %ld", "Exp: %ld"), (long)dead_ptr->exp);
	center_string(buf, tmp);
	put_str(buf, 12, 11);

	(void)sprintf(tmp, _("所持金: %ld", "AU: %ld"), (long)dead_ptr->au);
	center_string(buf, tmp);
	put_str(buf, 13, 11);

#ifdef JP
	/* 墓に刻む言葉をオリジナルより細かく表示 */
	if (streq(dead_ptr->died_from, "途中終了"))
	{
		strcpy(tmp, "<自殺>");
	}
	else if (streq(dead_ptr->died_from, "ripe"))
	{
		strcpy(tmp, "引退後に天寿を全う");
	}
	else if (streq(dead_ptr->died_from, "Seppuku"))
	{
		strcpy(tmp, "勝利の後、切腹");
	}
	else
	{
		roff_to_buf(dead_ptr->died_from, GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
		char *t;
		t = tmp + strlen(tmp) + 1;
		if (*t)
		{
			char dummy[80];
			strcpy(dummy, t); /* 2nd line */
			if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
			{
				for (t = dummy + strlen(dummy) - 2; iskanji(*(t - 1)); t--) /* Loop */;
				strcpy(t, "…");
			}
			else if (my_strstr(tmp, "『") && suffix(dummy, "』"))
			{
				char dummy2[80];
				char *name_head = my_strstr(tmp, "『");
				sprintf(dummy2, "%s%s", name_head, dummy);
				if (strlen(dummy2) <= GRAVE_LINE_WIDTH)
				{
					strcpy(dummy, dummy2);
					*name_head = '\0';
				}
			}
			else if (my_strstr(tmp, "「") && suffix(dummy, "」"))
			{
				char dummy2[80];
				char *name_head = my_strstr(tmp, "「");
				sprintf(dummy2, "%s%s", name_head, dummy);
				if (strlen(dummy2) <= GRAVE_LINE_WIDTH)
				{
					strcpy(dummy, dummy2);
					*name_head = '\0';
				}
			}

			center_string(buf, dummy);
			put_str(buf, 15, 11);
			extra_line = 1;
		}
	}

	center_string(buf, tmp);
	put_str(buf, 14, 11);

	if (!streq(dead_ptr->died_from, "ripe") && !streq(dead_ptr->died_from, "Seppuku"))
	{
		if (dead_ptr->current_floor_ptr->dun_level == 0)
		{
			concptr field_name = dead_ptr->town_num ? "街" : "荒野";
			if (streq(dead_ptr->died_from, "途中終了"))
			{
				sprintf(tmp, "%sで死んだ", field_name);
			}
			else
			{
				sprintf(tmp, "に%sで殺された", field_name);
			}
		}
		else
		{
			if (streq(dead_ptr->died_from, "途中終了"))
			{
				sprintf(tmp, "地下 %d 階で死んだ", (int)dead_ptr->current_floor_ptr->dun_level);
			}
			else
			{
				sprintf(tmp, "に地下 %d 階で殺された", (int)dead_ptr->current_floor_ptr->dun_level);
			}
		}

		center_string(buf, tmp);
		put_str(buf, 15 + extra_line, 11);
	}
#else
	(void)sprintf(tmp, "Killed on Level %d", dead_ptr->current_floor_ptr->dun_level);
	center_string(buf, tmp);
	put_str(buf, 14, 11);

	roff_to_buf(format("by %s.", dead_ptr->died_from), GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
	center_string(buf, tmp);
	char *t;
	put_str(buf, 15, 11);
	t = tmp + strlen(tmp) + 1;
	if (*t)
	{
		char dummy[80];
		strcpy(dummy, t); /* 2nd line */
		if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
		{
			int dummy_len = strlen(dummy);
			strcpy(dummy + MIN(dummy_len, GRAVE_LINE_WIDTH - 3), "...");
		}
		center_string(buf, dummy);
		put_str(buf, 16, 11);
	}
#endif
	time_t ct = time((time_t*)0);
	(void)sprintf(tmp, "%-.24s", ctime(&ct));
	center_string(buf, tmp);
	put_str(buf, 17, 11);
	msg_format(_("さようなら、%s!", "Goodbye, %s!"), dead_ptr->name);
}


/*!
 * todo handle_stuff、display_playerの引数は暫定。どのように設計し直すか少し考える
 * @brief 死亡、引退時の簡易ステータス表示
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param handle_stuff 更新処理チェックへのコールバック
 * @param file_character ステータスダンプへのコールバック
 * @param update_playtime プレイ時間更新処理へのコールバック
 * @param display_player ステータス表示へのコールバック
 * @return なし
 */
void show_info(player_type *creature_ptr, void(*handle_stuff)(player_type*), errr(*file_character)(player_type*, concptr), void(*update_playtime)(void), void(*display_player)(player_type*, int))
{
	object_type *o_ptr;
	for (int i = 0; i < INVEN_TOTAL; i++)
	{
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		object_aware(creature_ptr, o_ptr);
		object_known(o_ptr);
	}

	store_type *st_ptr;
	for (int i = 1; i < max_towns; i++)
	{
		st_ptr = &town_info[i].store[STORE_HOME];

		/* Hack -- Know everything in the home */
		for (int j = 0; j < st_ptr->stock_num; j++)
		{
			o_ptr = &st_ptr->stock[j];
			if (!o_ptr->k_idx) continue;

			/* Aware and Known */
			object_aware(creature_ptr, o_ptr);
			object_known(o_ptr);
		}
	}

	creature_ptr->update |= (PU_BONUS);
	handle_stuff(creature_ptr);
	flush();
	msg_erase();
	prt(_("キャラクターの記録をファイルに書き出すことができます。", "You may now dump a character record to one or more files."), 21, 0);
	prt(_("リターンキーでキャラクターを見ます。ESCで中断します。", "Then, hit RETURN to see the character, or ESC to abort."), 22, 0);
	while (TRUE)
	{
		char out_val[160];
		put_str(_("ファイルネーム: ", "Filename: "), 23, 0);
		strcpy(out_val, "");
		if (!askfor(out_val, 60)) return;
		if (!out_val[0]) break;
		screen_save();
		(void)file_character(creature_ptr, out_val);
		screen_load();
	}

	update_playtime();
	display_player(creature_ptr, 0);
	prt(_("何かキーを押すとさらに情報が続きます (ESCで中断): ", "Hit any key to see more information (ESC to abort): "), 23, 0);
	if (inkey() == ESCAPE) return;

	if (creature_ptr->equip_cnt)
	{
		Term_clear();
		(void)show_equipment(creature_ptr, 0, USE_FULL, 0);
		prt(_("装備していたアイテム: -続く-", "You are using: -more-"), 0, 0);
		if (inkey() == ESCAPE) return;
	}

	if (creature_ptr->inven_cnt)
	{
		Term_clear();
		(void)show_inventory(creature_ptr, 0, USE_FULL, 0);
		prt(_("持っていたアイテム: -続く-", "You are carrying: -more-"), 0, 0);

		if (inkey() == ESCAPE) return;
	}

	for (int l = 1; l < max_towns; l++)
	{
		st_ptr = &town_info[l].store[STORE_HOME];
		if (st_ptr->stock_num == 0) continue;
		for (int i = 0, k = 0; i < st_ptr->stock_num; k++)
		{
			Term_clear();
			for (int j = 0; (j < 12) && (i < st_ptr->stock_num); j++, i++)
			{
				GAME_TEXT o_name[MAX_NLEN];
				char tmp_val[80];
				o_ptr = &st_ptr->stock[i];
				sprintf(tmp_val, "%c) ", I2A(j));
				prt(tmp_val, j + 2, 4);
				object_desc(creature_ptr, o_name, o_ptr, 0);
				c_put_str(tval_to_attr[o_ptr->tval], o_name, j + 2, 7);
			}

			prt(format(_("我が家に置いてあったアイテム ( %d ページ): -続く-", "Your home contains (page %d): -more-"), k + 1), 0, 0);
			if (inkey() == ESCAPE) return;
		}
	}
}
