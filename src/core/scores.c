/*!
 * @file scores.c
 * @brief ハイスコア処理 / Highscores handling
 * @date 2014/07/14
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "core/scores.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/turn-compensator.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "io/report.h"
#include "io/signal-handlers.h"
#include "io/uid-checker.h"
#include "io/write-diary.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#include "locale/japanese.h"
#endif

 /*
  * The "highscore" file descriptor, if available.
  */
int highscore_fd = -1;

/*!
 * @brief i番目のスコア情報にバッファ位置をシークする / Seek score 'i' in the highscore file
 * @param i スコア情報ID
 * @return 問題がなければ0を返す
 */
static int highscore_seek(int i)
{
	/* Seek for the requested record */
	return (fd_seek(highscore_fd, (huge)(i) * sizeof(high_score)));
}


/*!
 * @brief 所定ポインタからスコア情報を読み取る / Read one score from the highscore file
 * @param score スコア情報参照ポインタ
 * @return エラーコード
 */
static errr highscore_read(high_score *score)
{
	/* Read the record, note failure */
	return (fd_read(highscore_fd, (char*)(score), sizeof(high_score)));
}


/*!
 * @brief 所定ポインタへスコア情報を書き込む / Write one score to the highscore file
 * @param score スコア情報参照ポインタ
 * @return エラーコード(問題がなければ0を返す)
 */
static int highscore_write(high_score *score)
{
	/* Write the record, note failure */
	return (fd_write(highscore_fd, (char*)(score), sizeof(high_score)));
}


/*!
 * @brief スコア情報を全て得るまで繰り返し取得する / Just determine where a new score *would* be placed
 * @param score スコア情報参照ポインタ
 * @return 正常ならば(MAX_HISCORES - 1)、問題があれば-1を返す
 */
static int highscore_where(high_score *score)
{
	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return -1;

	/* Go to the start of the highscore file */
	if (highscore_seek(0)) return -1;

	/* Read until we get to a higher score */
	high_score the_score;
	int my_score = atoi(score->pts);
	for (int i = 0; i < MAX_HISCORES; i++)
	{
		int old_score;
		if (highscore_read(&the_score)) return (i);
		old_score = atoi(the_score.pts);
		if (my_score > old_score) return (i);
	}

	/* The "last" entry is always usable */
	return MAX_HISCORES - 1;
}


/*!
 * @brief スコア情報をバッファの末尾に追加する / Actually place an entry into the high score file
 * @param score スコア情報参照ポインタ
 * @return 正常ならば書き込んだスロット位置、問題があれば-1を返す / Return the location (0 is best) or -1 on "failure"
 */
static int highscore_add(high_score *score)
{
	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return -1;

	/* Determine where the score should go */
	int slot = highscore_where(score);

	/* Hack -- Not on the list */
	if (slot < 0) return -1;

	/* Hack -- prepare to dump the new score */
	high_score the_score = (*score);

	/* Slide all the scores down one */
	bool done = FALSE;
	high_score tmpscore;
	for (int i = slot; !done && (i < MAX_HISCORES); i++)
	{
		/* Read the old guy, note errors */
		if (highscore_seek(i)) return -1;
		if (highscore_read(&tmpscore)) done = TRUE;

		/* Back up and dump the score we were holding */
		if (highscore_seek(i)) return -1;
		if (highscore_write(&the_score)) return -1;

		/* Hack -- Save the old score, for the next pass */
		the_score = tmpscore;
	}

	/* Return location used */
	return slot;
}


/*!
 * @brief 指定された順位範囲でスコアを並べて表示する / Display the scores in a given range.
 * @param from 順位先頭
 * @param to 順位末尾
 * @param note 黄色表示でハイライトする順位
 * @param score スコア配列参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Assumes the high score list is already open.
 * Only five entries per line, too much info.
 *
 * Mega-Hack -- allow "fake" entry at the given position.
 * </pre>
 */
void display_scores_aux(int from, int to, int note, high_score *score)
{
	int i, j, k, n, place;
	TERM_COLOR attr;

	high_score the_score;

	GAME_TEXT out_val[256];
	GAME_TEXT tmp_val[160];

	TERM_LEN wid, hgt, per_screen;

	term_get_size(&wid, &hgt);
	per_screen = (hgt - 4) / 4;

	/* Paranoia -- it may not have opened */
	if (highscore_fd < 0) return;


	/* Assume we will show the first 10 */
	if (from < 0) from = 0;
	if (to < 0) to = 10;
	if (to > MAX_HISCORES) to = MAX_HISCORES;


	/* Seek to the beginning */
	if (highscore_seek(0)) return;

	/* Hack -- Count the high scores */
	for (i = 0; i < MAX_HISCORES; i++)
	{
		if (highscore_read(&the_score)) break;
	}

	/* Hack -- allow "fake" entry to be last */
	if ((note == i) && score) i++;

	/* Forget about the last entries */
	if (i > to) i = to;


	/* Show per_screen per page, until "done" */
	for (k = from, place = k+1; k < i; k += per_screen)
	{
		term_clear();

		/* Title */
		put_str(_("                変愚蛮怒: 勇者の殿堂", "                Hengband Hall of Fame"), 0, 0);

		/* Indicate non-top scores */
		if (k > 0)
		{
			sprintf(tmp_val, _("( %d 位以下 )", "(from position %d)"), k + 1);
			put_str(tmp_val, 0, 40);
		}

		/* Dump per_screen entries */
		for (j = k, n = 0; j < i && n < per_screen; place++, j++, n++)
		{
			int pr, pc, pa, clev, mlev, cdun, mdun;

			concptr user, gold, when, aged;


			/* Hack -- indicate death in yellow */
			attr = (j == note) ? TERM_YELLOW : TERM_WHITE;


			/* Mega-Hack -- insert a "fake" record */
			if ((note == j) && score)
			{
				the_score = (*score);
				attr = TERM_L_GREEN;
				score = NULL;
				note = -1;
				j--;
			}

			/* Read a normal record */
			else
			{
				/* Read the proper record */
				if (highscore_seek(j)) break;
				if (highscore_read(&the_score)) break;
			}

			/* Extract the race/class */
			pr = atoi(the_score.p_r);
			pc = atoi(the_score.p_c);
			pa = atoi(the_score.p_a);

			/* Extract the level info */
			clev = atoi(the_score.cur_lev);
			mlev = atoi(the_score.max_lev);
			cdun = atoi(the_score.cur_dun);
			mdun = atoi(the_score.max_dun);

			/* Hack -- extract the gold and such */
			for (user = the_score.uid; iswspace(*user); user++) /* loop */;
			for (when = the_score.day; iswspace(*when); when++) /* loop */;
			for (gold = the_score.gold; iswspace(*gold); gold++) /* loop */;
			for (aged = the_score.turns; iswspace(*aged); aged++) /* loop */;

			/* Clean up standard encoded form of "when" */
			if ((*when == '@') && strlen(when) == 9)
			{
				sprintf(tmp_val, "%.4s-%.2s-%.2s",
					when + 1, when + 5, when + 7);
				when = tmp_val;
			}

			/* Dump some info */
#ifdef JP
/*sprintf(out_val, "%3d.%9s  %s%s%sという名の%sの%s (レベル %d)", */
			sprintf(out_val, "%3d.%9s  %s%s%s - %s%s (レベル %d)",
				place, the_score.pts,
				personality_info[pa].title, (personality_info[pa].no ? "の" : ""),
				the_score.who,
				race_info[pr].title, class_info[pc].title,
				clev);

#else
			sprintf(out_val, "%3d.%9s  %s %s the %s %s, Level %d",
				place, the_score.pts,
				personality_info[pa].title,
				the_score.who, race_info[pr].title, class_info[pc].title,
				clev);
#endif


			/* Append a "maximum level" */
			if (mlev > clev) strcat(out_val, format(_(" (最高%d)", " (Max %d)"), mlev));

			/* Dump the first line */
			c_put_str(attr, out_val, n*4 + 2, 0);

			/* Another line of info */
#ifdef JP
			if (mdun != 0)
				sprintf(out_val, "    最高%3d階", mdun);
			else
				sprintf(out_val, "             ");


			/* 死亡原因をオリジナルより細かく表示 */
			if (streq(the_score.how, "yet"))
			{
				sprintf(out_val+13, "  まだ生きている (%d%s)",
				       cdun, "階");
			}
			else
			if (streq(the_score.how, "ripe"))
			{
				sprintf(out_val+13, "  勝利の後に引退 (%d%s)",
					cdun, "階");
			}
			else if (streq(the_score.how, "Seppuku"))
			{
				sprintf(out_val+13, "  勝利の後に切腹 (%d%s)",
					cdun, "階");
			}
			else
			{
				codeconv(the_score.how);

				/* Some people die outside of the dungeon */
				if (!cdun)
					sprintf(out_val+13, "  地上で%sに殺された", the_score.how);
				else
					sprintf(out_val+13, "  %d階で%sに殺された",
						cdun, the_score.how);
			}

#else
			/* Some people die outside of the dungeon */
			if (!cdun)
				sprintf(out_val, 
					"               Killed by %s on the surface",
					the_score.how);
			else
				sprintf(out_val, 
					"               Killed by %s on %s %d",
					the_score.how, "Dungeon Level", cdun);

			/* Append a "maximum level" */
			if (mdun > cdun) strcat(out_val, format(" (Max %d)", mdun));
#endif

			/* Dump the info */
			c_put_str(attr, out_val, n*4 + 3, 0);

			/* And still another line of info */
#ifdef JP
			{
				char buf[11];

				/* 日付を 19yy/mm/dd の形式に変更する */
				if (strlen(when) == 8 && when[2] == '/' && when[5] == '/') {
					sprintf(buf, "%d%s/%.5s", 19 + (when[6] < '8'), when + 6, when);
					when = buf;
				}
				sprintf(out_val,
						"        (ユーザー:%s, 日付:%s, 所持金:%s, ターン:%s)",
						user, when, gold, aged);
			}

#else
			sprintf(out_val,
				"               (User %s, Date %s, Gold %s, Turn %s).",
				user, when, gold, aged);
#endif

			c_put_str(attr, out_val, n*4 + 4, 0);
		}


		/* Wait for response */
		prt(_("[ ESCで中断, その他のキーで続けます ]", "[Press ESC to quit, any other key to continue.]"), hgt - 1, _(21, 17));

		j = inkey();
		prt("", hgt - 1, 0);

		/* Hack -- notice Escape */
		if (j == ESCAPE) break;
	}
}


/*!
 * @brief スコア表示処理メインルーチン / Hack -- Display the scores in a given range and quit.
 * @param from 順位先頭
 * @param to 順位末尾
 * @return なし
 * @details
 * <pre>
 * This function is only called from "main.c" when the user asks
 * to see the "high scores".
 * </pre>
 */
void display_scores(int from, int to)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

	/* Open the binary high score file, for reading */
	highscore_fd = fd_open(buf, O_RDONLY);

	/* Paranoia -- No score file */
	if (highscore_fd < 0) quit(_("スコア・ファイルが使用できません。", "Score file unavailable."));
	term_clear();

	/* Display the scores */
	display_scores_aux(from, to, -1, NULL);

	/* Shut the high score file */
	(void)fd_close(highscore_fd);

	/* Forget the high score fd */
	highscore_fd = -1;

	/* Quit */
	quit(NULL);
}


/*!
 * todo プリプロが邪魔していて最初のif文を削除すると到達不能コードが発生する
 * @brief スコアサーバへの転送処理
 * @param current_player_ptr プレーヤーへの参照ポインタ
 * @param do_send 実際に転送ア処置を行うか否か
 * @return 転送が成功したらTRUEを返す
 */
bool send_world_score(player_type *current_player_ptr, bool do_send, void(*update_playtime)(void), display_player_pf display_player)
{
#ifdef WORLD_SCORE
	if (send_score && do_send)
	{
		if (easy_band)
		{
			msg_print(_("初心者モードではワールドスコアに登録できません。",
				"Since you are in the Easy Mode, you cannot send score to world score server."));
			return TRUE;
		}
		
		bool is_registration = get_check_strict(current_player_ptr,
                    _("スコアをスコア・サーバに登録しますか? ", "Do you send score to the world score server? "), (CHECK_NO_ESCAPE | CHECK_NO_HISTORY));
		if (!is_registration) return FALSE;

		errr err;
		prt("", 0, 0);
		prt(_("送信中．．", "Sending..."), 0, 0);
		term_fresh();
		screen_save();
		err = report_score(current_player_ptr, update_playtime, display_player);
		screen_load();
		if (err) return FALSE;

		prt(_("完了。何かキーを押してください。", "Completed.  Hit any key."), 0, 0);
		(void)inkey();
	}
#endif
	return TRUE;
}


/*!
 * @brief スコアの過去二十位内ランキングを表示する
 * Enters a players name on a hi-score table, if "legal", and in any
 * case, displays some relevant portion of the high score list.
 * @param current_player_ptr スコアに適用するための現在プレイヤークリーチャー参照ポインタ
 * @return エラーコード
 * @details
 * Assumes "signals_ignore_tstp()" has been called.
 */
errr top_twenty(player_type *current_player_ptr)
{
	high_score the_score;
	(void)WIPE(&the_score, high_score);

	/* Save the version */
	sprintf(the_score.what, "%u.%u.%u",
		FAKE_VER_MAJOR, FAKE_VER_MINOR, FAKE_VER_PATCH);

	/* Calculate and save the points */
	sprintf(the_score.pts, "%9ld", (long)calc_score(current_player_ptr));
	the_score.pts[9] = '\0';

	/* Save the current gold */
	sprintf(the_score.gold, "%9lu", (long)current_player_ptr->au);
	the_score.gold[9] = '\0';

	/* Save the current turn */
	sprintf(the_score.turns, "%9lu", (long)turn_real(current_player_ptr, current_world_ptr->game_turn));
	the_score.turns[9] = '\0';

	time_t ct = time((time_t*)0);

	/* Save the date in standard encoded form (9 chars) */
	strftime(the_score.day, 10, "@%Y%m%d", localtime(&ct));

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", current_player_ptr->name);

	/* Save the player info */
	sprintf(the_score.uid, "%7u", current_player_ptr->player_uid);
	sprintf(the_score.sex, "%c", (current_player_ptr->psex ? 'm' : 'f'));
	sprintf(the_score.p_r, "%2d", MIN(current_player_ptr->prace, MAX_RACES));
	sprintf(the_score.p_c, "%2d", MIN(current_player_ptr->pclass, MAX_CLASS));
	sprintf(the_score.p_a, "%2d", MIN(current_player_ptr->pseikaku, MAX_PERSONALITIES));

	/* Save the level and such */
	sprintf(the_score.cur_lev, "%3d", MIN((u16b)current_player_ptr->lev, 999));
	sprintf(the_score.cur_dun, "%3d", (int)current_player_ptr->current_floor_ptr->dun_level);
	sprintf(the_score.max_lev, "%3d", MIN((u16b)current_player_ptr->max_plv, 999));
	sprintf(the_score.max_dun, "%3d", (int)max_dlv[current_player_ptr->dungeon_idx]);

	/* Save the cause of death (31 chars) */
	if (strlen(current_player_ptr->died_from) >= sizeof(the_score.how))
	{
#ifdef JP
		angband_strcpy(the_score.how, current_player_ptr->died_from, sizeof(the_score.how) - 2);
		strcat(the_score.how, "…");
#else
		angband_strcpy(the_score.how, current_player_ptr->died_from, sizeof(the_score.how) - 3);
		strcat(the_score.how, "...");
#endif
	}
	else
	{
		strcpy(the_score.how, current_player_ptr->died_from);
	}

	/* Grab permissions */
	safe_setuid_grab(current_player_ptr);

	/* Lock (for writing) the highscore file, or fail */
	errr err = fd_lock(highscore_fd, F_WRLCK);

	/* Drop permissions */
	safe_setuid_drop();

	if (err) return 1;

	/* Add a new entry to the score list, see where it went */
	int j = highscore_add(&the_score);

	/* Grab permissions */
	safe_setuid_grab(current_player_ptr);

	/* Unlock the highscore file, or fail */
	err = fd_lock(highscore_fd, F_UNLCK);

	/* Drop permissions */
	safe_setuid_drop();

	if (err) return 1;

	/* Hack -- Display the top fifteen scores */
	if (j < 10)
	{
		display_scores_aux(0, 15, j, NULL);
		return 0;
	}

	/* Display the scores surrounding the player */
	display_scores_aux(0, 5, j, NULL);
	display_scores_aux(j - 2, j + 7, j, NULL);
	return 0;
}


/*!
 * @brief プレイヤーの現在のスコアをランキングに挟む /
 * Predict the players location, and display it.
 * @return エラーコード
 */
errr predict_score(player_type *current_player_ptr)
{
	high_score the_score;

	/* No score file */
	if (highscore_fd < 0)
	{
		msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
		msg_print(NULL);
		return 0;
	}

	/* Save the version */
	sprintf(the_score.what, "%u.%u.%u",
		FAKE_VER_MAJOR, FAKE_VER_MINOR, FAKE_VER_PATCH);

	/* Calculate and save the points */
	sprintf(the_score.pts, "%9ld", (long)calc_score(current_player_ptr));

	/* Save the current gold */
	sprintf(the_score.gold, "%9lu", (long)current_player_ptr->au);

	/* Save the current turn */
	sprintf(the_score.turns, "%9lu", (long)turn_real(current_player_ptr, current_world_ptr->game_turn));

	/* Hack -- no time needed */
	strcpy(the_score.day, _("今日", "TODAY"));

	/* Save the player name (15 chars) */
	sprintf(the_score.who, "%-.15s", current_player_ptr->name);

	/* Save the player info */
	sprintf(the_score.uid, "%7u", current_player_ptr->player_uid);
	sprintf(the_score.sex, "%c", (current_player_ptr->psex ? 'm' : 'f'));
	sprintf(the_score.p_r, "%2d", MIN(current_player_ptr->prace, MAX_RACES));
	sprintf(the_score.p_c, "%2d", MIN(current_player_ptr->pclass, MAX_CLASS));
	sprintf(the_score.p_a, "%2d", MIN(current_player_ptr->pseikaku, MAX_PERSONALITIES));

	/* Save the level and such */
	sprintf(the_score.cur_lev, "%3d", MIN((u16b)current_player_ptr->lev, 999));
	sprintf(the_score.cur_dun, "%3d", (int)current_player_ptr->current_floor_ptr->dun_level);
	sprintf(the_score.max_lev, "%3d", MIN((u16b)current_player_ptr->max_plv, 999));
	sprintf(the_score.max_dun, "%3d", (int)max_dlv[current_player_ptr->dungeon_idx]);

	/* Hack -- no cause of death */
	/* まだ死んでいないときの識別文字 */
	strcpy(the_score.how, _("yet", "nobody (yet!)"));

	/* See where the entry would be placed */
	int j = highscore_where(&the_score);

	/* Hack -- Display the top fifteen scores */
	if (j < 10)
	{
		display_scores_aux(0, 15, j, &the_score);
		return 0;
	}

	display_scores_aux(0, 5, -1, NULL);
	display_scores_aux(j - 2, j + 7, j, &the_score);
	return 0;
}


/*!
 * @brief スコアランキングの簡易表示 /
 * show_highclass - selectively list highscores based on class -KMW-
 * @return なし
 */
void show_highclass(player_type *current_player_ptr)
{
	screen_save();
	char buf[1024], out_val[256];
	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

	highscore_fd = fd_open(buf, O_RDONLY);

	if (highscore_fd < 0)
	{
		msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
		msg_print(NULL);
		return;
	}

	if (highscore_seek(0)) return;

	high_score the_score;
	for (int i = 0; i < MAX_HISCORES; i++)
		if (highscore_read(&the_score)) break;

	int m = 0;
	int j = 0;
	PLAYER_LEVEL clev = 0;
	int pr;
	while ((m < 9) && (j < MAX_HISCORES))
	{
		if (highscore_seek(j)) break;
		if (highscore_read(&the_score)) break;
		pr = atoi(the_score.p_r);
		clev = (PLAYER_LEVEL)atoi(the_score.cur_lev);

#ifdef JP
		sprintf(out_val, "   %3d) %sの%s (レベル %2d)",
		    (m + 1), race_info[pr].title,the_score.who, clev);
#else
		sprintf(out_val, "%3d) %s the %s (Level %2d)",
		    (m + 1), the_score.who, race_info[pr].title, clev);
#endif

		prt(out_val, (m + 7), 0);
		m++;
		j++;
	}

#ifdef JP
	sprintf(out_val, "あなた) %sの%s (レベル %2d)",
	    race_info[current_player_ptr->prace].title,current_player_ptr->name, current_player_ptr->lev);
#else
	sprintf(out_val, "You) %s the %s (Level %2d)",
	    current_player_ptr->name, race_info[current_player_ptr->prace].title, current_player_ptr->lev);
#endif

	prt(out_val, (m + 8), 0);

	(void)fd_close(highscore_fd);
	highscore_fd = -1;
	prt(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"),0,0);

	(void)inkey();

	for (j = 5; j < 18; j++) prt("", j, 0);
	screen_load();
}


/*!
 * @brief スコアランキングの簡易表示(種族毎)サブルーチン /
 * Race Legends -KMW-
 * @param race_num 種族ID
 * @return なし
 */
void race_score(player_type *current_player_ptr, int race_num)
{
	register int i = 0, j, m = 0;
	int pr, clev, lastlev;
	high_score the_score;
	char buf[1024], out_val[256], tmp_str[80];

	lastlev = 0;

	/* rr9: TODO - pluralize the race */
	sprintf(tmp_str,_("最高の%s", "The Greatest of all the %s"), race_info[race_num].title);

	prt(tmp_str, 5, 15);
	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

	highscore_fd = fd_open(buf, O_RDONLY);

	if (highscore_fd < 0)
	{
		msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
		msg_print(NULL);
		return;
	}

	if (highscore_seek(0)) return;

	for (i = 0; i < MAX_HISCORES; i++)
	{
		if (highscore_read(&the_score)) break;
	}

	m = 0;
	j = 0;

	while ((m < 10) || (j < MAX_HISCORES))
	{
		if (highscore_seek(j)) break;
		if (highscore_read(&the_score)) break;
		pr = atoi(the_score.p_r);
		clev = atoi(the_score.cur_lev);

		if (pr == race_num)
		{
#ifdef JP
		sprintf(out_val, "   %3d) %sの%s (レベル %2d)",
			    (m + 1), race_info[pr].title, 
				the_score.who,clev);
#else
			sprintf(out_val, "%3d) %s the %s (Level %3d)",
			    (m + 1), the_score.who,
			race_info[pr].title, clev);
#endif

			prt(out_val, (m + 7), 0);
			m++;
			lastlev = clev;
		}
		j++;
	}

	/* add player if qualified */
	if ((current_player_ptr->prace == race_num) && (current_player_ptr->lev >= lastlev))
	{
#ifdef JP
	sprintf(out_val, "あなた) %sの%s (レベル %2d)",
		     race_info[current_player_ptr->prace].title,current_player_ptr->name, current_player_ptr->lev);
#else
		sprintf(out_val, "You) %s the %s (Level %3d)",
		    current_player_ptr->name, race_info[current_player_ptr->prace].title, current_player_ptr->lev);
#endif

		prt(out_val, (m + 8), 0);
	}

	(void)fd_close(highscore_fd);
	highscore_fd = -1;
}


/*!
 * @brief スコアランキングの簡易表示(種族毎)メインルーチン /
 * Race Legends -KMW-
 * @return なし
 */
void race_legends(player_type *current_player_ptr)
{
	for (int i = 0; i < MAX_RACES; i++)
	{
		race_score(current_player_ptr, i);
		msg_print(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"));
		msg_print(NULL);
		for (int j = 5; j < 19; j++)
			prt("", j, 0);
	}
}


/*!
 * @brief 勝利者用の引退演出処理 /
 * Change the player into a King! -RAK-
 * @return なし
 */
void kingly(player_type *winner_ptr)
{
	TERM_LEN wid, hgt;
	TERM_LEN cx, cy;
	bool seppuku = streq(winner_ptr->died_from, "Seppuku");

	/* Hack -- retire in town */
	winner_ptr->current_floor_ptr->dun_level = 0;

	/* Fake death */
	if (!seppuku)
		/* 引退したときの識別文字 */
		(void)strcpy(winner_ptr->died_from, _("ripe", "Ripe Old Age"));

	/* Restore the experience */
	winner_ptr->exp = winner_ptr->max_exp;

	/* Restore the level */
	winner_ptr->lev = winner_ptr->max_plv;

	term_get_size(&wid, &hgt);
	cy = hgt / 2;
	cx = wid / 2;

	/* Hack -- Instant Gold */
	winner_ptr->au += 10000000L;
	term_clear();

	/* Display a crown */
	put_str("#", cy - 11, cx - 1);
	put_str("#####", cy - 10, cx - 3);
	put_str("#", cy - 9, cx - 1);
	put_str(",,,  $$$  ,,,", cy - 8, cx - 7);
	put_str(",,=$   \"$$$$$\"   $=,,", cy - 7, cx - 11);
	put_str(",$$        $$$        $$,", cy - 6, cx - 13);
	put_str("*>         <*>         <*", cy - 5, cx - 13);
	put_str("$$         $$$         $$", cy - 4, cx - 13);
	put_str("\"$$        $$$        $$\"", cy - 3, cx - 13);
	put_str("\"$$       $$$       $$\"", cy - 2, cx - 12);
	put_str("*#########*#########*", cy - 1, cx - 11);
	put_str("*#########*#########*", cy, cx - 11);

	/* Display a message */
#ifdef JP
	put_str("Veni, Vidi, Vici!", cy + 3, cx - 9);
	put_str("来た、見た、勝った！", cy + 4, cx - 10);
	put_str(format("偉大なる%s万歳！", sp_ptr->winner), cy + 5, cx - 11);
#else
	put_str("Veni, Vidi, Vici!", cy + 3, cx - 9);
	put_str("I came, I saw, I conquered!", cy + 4, cx - 14);
	put_str(format("All Hail the Mighty %s!", sp_ptr->winner), cy + 5, cx - 13);
#endif

	/* If player did Seppuku, that is already written in playrecord */
	if (!seppuku)
	{
		exe_write_diary(winner_ptr, DIARY_DESCRIPTION, 0, _("ダンジョンの探索から引退した。", "retired exploring dungeons."));
		exe_write_diary(winner_ptr, DIARY_GAMESTART, 1, _("-------- ゲームオーバー --------", "--------   Game  Over   --------"));
		exe_write_diary(winner_ptr, DIARY_DESCRIPTION, 1, "\n\n\n\n");
	}

	/* Flush input */
	flush();

	/* Wait for response */
	pause_line(hgt - 1);
}


/*!
 * @brief スコアファイル出力
 * Display some character info
 * @return なし
 */
bool check_score(player_type *current_player_ptr)
{
	term_clear();

	/* No score file */
	if (highscore_fd < 0)
	{
		msg_print(_("スコア・ファイルが使用できません。", "Score file unavailable."));
		msg_print(NULL);
		return FALSE;
	}

	/* Wizard-mode pre-empts scoring */
	if (current_world_ptr->noscore & 0x000F)
	{
		msg_print(_("ウィザード・モードではスコアが記録されません。", "Score not registered for wizards."));
		msg_print(NULL);
		return FALSE;
	}

	/* Cheaters are not scored */
	if (current_world_ptr->noscore & 0xFF00)
	{
		msg_print(_("詐欺をやった人はスコアが記録されません。", "Score not registered for cheaters."));
		msg_print(NULL);
		return FALSE;
	}

	/* Interupted */
	if (!current_world_ptr->total_winner && streq(current_player_ptr->died_from, _("強制終了", "Interrupting")))
	{
		msg_print(_("強制終了のためスコアが記録されません。", "Score not registered due to interruption."));
		msg_print(NULL);
		return FALSE;
	}

	/* Quitter */
	if (!current_world_ptr->total_winner && streq(current_player_ptr->died_from, _("途中終了", "Quitting")))
	{
		msg_print(_("途中終了のためスコアが記録されません。", "Score not registered due to quitting."));
		msg_print(NULL);
		return FALSE;
	}
	return TRUE;
}
