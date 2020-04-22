/*!
* @file cmd-process-screen.c
* @brief 記念撮影のセーブとロード
* @date 2020/04/22
* @Author Hourier
*/

#include "angband.h"
#include "cmd/cmd-process-screen.h"
#include "cmd/cmd-draw.h"
#include "files.h"
#include "gameterm.h"

// Encode the screen colors
static char hack[17] = "dwsorgbuDWvyRGBU";

void do_cmd_save_screen_html_aux(char *filename, int message)
{
	concptr tags[4] = {
		"HEADER_START:",
		"HEADER_END:",
		"FOOTER_START:",
		"FOOTER_END:",
	};
	concptr html_head[] = {
		"<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
		"<pre>",
		0,
	};
	concptr html_foot[] = {
		"</pre>\n",
		"</body>\n</html>\n",
		0,
	};

	TERM_LEN wid, hgt;
	Term_get_size(&wid, &hgt);
	FILE_TYPE(FILE_TYPE_TEXT);
	FILE *fff;
	fff = my_fopen(filename, "w");
	if (!fff)
	{
		if (message)
		{
			msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename);
			msg_print(NULL);
		}

		return;
	}

	if (message) screen_save();

	char buf[2048];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "htmldump.prf");
	FILE *tmpfff;
	tmpfff = my_fopen(buf, "r");
	if (!tmpfff)
	{
		for (int i = 0; html_head[i]; i++)
			fputs(html_head[i], fff);
	}
	else
	{
		bool is_first_line = TRUE;
		while (!my_fgets(tmpfff, buf, sizeof(buf)))
		{
			if (is_first_line)
			{
				if (strncmp(buf, tags[0], strlen(tags[0])) == 0)
					is_first_line = FALSE;
			}
			else
			{
				if (strncmp(buf, tags[1], strlen(tags[1])) == 0)
					break;
				fprintf(fff, "%s\n", buf);
			}
		}
	}

	for (TERM_LEN y = 0; y < hgt; y++)
	{
		if (y != 0) fprintf(fff, "\n");

		TERM_COLOR a = 0, old_a = 0;
		char c = ' ';
		for (TERM_LEN x = 0; x < wid - 1; x++)
		{
			concptr cc = NULL;
			(void)(Term_what(x, y, &a, &c));
			switch (c)
			{
			case '&': cc = "&amp;"; break;
			case '<': cc = "&lt;"; break;
			case '>': cc = "&gt;"; break;
#ifdef WINDOWS
			case 0x1f: c = '.'; break;
			case 0x7f: c = (a == 0x09) ? '%' : '#'; break;
#endif
			}

			a = a & 0x0F;
			if ((y == 0 && x == 0) || a != old_a)
			{
				int rv = angband_color_table[a][1];
				int gv = angband_color_table[a][2];
				int bv = angband_color_table[a][3];
				fprintf(fff, "%s<font color=\"#%02x%02x%02x\">",
					((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
				old_a = a;
			}

			if (cc)
				fprintf(fff, "%s", cc);
			else
				fprintf(fff, "%c", c);
		}
	}

	fprintf(fff, "</font>");
	if (!tmpfff)
	{
		for (int i = 0; html_foot[i]; i++)
			fputs(html_foot[i], fff);
	}
	else
	{
		rewind(tmpfff);
		bool is_first_line = TRUE;
		while (!my_fgets(tmpfff, buf, sizeof(buf)))
		{
			if (is_first_line)
			{
				if (strncmp(buf, tags[2], strlen(tags[2])) == 0)
					is_first_line = FALSE;
			}
			else
			{
				if (strncmp(buf, tags[3], strlen(tags[3])) == 0)
					break;
				fprintf(fff, "%s\n", buf);
			}
		}

		my_fclose(tmpfff);
	}

	fprintf(fff, "\n");
	my_fclose(fff);
	if (message)
	{
		msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
		msg_print(NULL);
	}

	if (message)
		screen_load();
}


/*
 * Hack -- save a screen dump to a file
 */
static void do_cmd_save_screen_html(void)
{
	char buf[1024], tmp[256] = "screen.html";

	if (!get_string(_("ファイル名: ", "File name: "), tmp, 80))
		return;
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

	msg_print(NULL);

	do_cmd_save_screen_html_aux(buf, 1);
}


/*!
 * @brief 記念撮影の方式を問い合わせる
 * @param html_dump HTMLダンプするか否か
 * @return ダンプするならTRUE、キャンセルならFALSE
 */
static bool ask_html_dump(bool *html_dump)
{
	while (TRUE)
	{
		char c = inkey();
		if (c == 'Y' || c == 'y')
		{
			*html_dump = FALSE;
			return TRUE;
		}
		
		if (c == 'H' || c == 'h')
		{
			*html_dump = TRUE;
			return TRUE;
		}

		prt("", 0, 0);
		return FALSE;
	}

	// コンパイル警告対応.
	return FALSE;
}


/*!
 * @brief 記念撮影のためにグラフィック使用をOFFにする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param handle_stuff 画面更新用の関数ポインタ
 * @return 記念撮影直前のグラフィックオプション
 */
static bool update_use_graphics(player_type *creature_ptr, void(*handle_stuff)(player_type*))
{
	if (!use_graphics) return TRUE;

	use_graphics = FALSE;
	reset_visuals(creature_ptr);
	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
	(*handle_stuff)(creature_ptr);
	return FALSE;
}


/*
 * Save a screen dump to a file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_save_screen(player_type *creature_ptr, void(*handle_stuff)(player_type*))
{
	prt(_("記念撮影しますか？ [(y)es/(h)tml/(n)o] ", "Save screen dump? [(y)es/(h)tml/(n)o] "), 0, 0);
	bool html_dump;
	if (!ask_html_dump(&html_dump)) return;

	int wid, hgt;
	Term_get_size(&wid, &hgt);

	bool old_use_graphics = update_use_graphics(creature_ptr, handle_stuff);

	if (html_dump)
	{
		do_cmd_save_screen_html();
		do_cmd_redraw(creature_ptr);
	}
	else
	{
		TERM_LEN y, x;
		TERM_COLOR a = 0;
		SYMBOL_CODE c = ' ';
		FILE *fff;
		char buf[1024];
		path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");
		FILE_TYPE(FILE_TYPE_TEXT);
		fff = my_fopen(buf, "w");
		if (!fff)
		{
			msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
			msg_print(NULL);
			return;
		}

		screen_save();
		for (y = 0; y < hgt; y++)
		{
			for (x = 0; x < wid - 1; x++)
			{
				(void)(Term_what(x, y, &a, &c));
				buf[x] = c;
			}

			buf[x] = '\0';
			fprintf(fff, "%s\n", buf);
		}

		fprintf(fff, "\n");
		for (y = 0; y < hgt; y++)
		{
			for (x = 0; x < wid - 1; x++)
			{
				(void)(Term_what(x, y, &a, &c));
				buf[x] = hack[a & 0x0F];
			}

			buf[x] = '\0';
			fprintf(fff, "%s\n", buf);
		}

		fprintf(fff, "\n");
		my_fclose(fff);
		msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
		msg_print(NULL);
		screen_load();
	}

	if (!old_use_graphics) return;

	use_graphics = TRUE;
	reset_visuals(creature_ptr);
	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
	handle_stuff(creature_ptr);
}


/*
 * @brief Load a screen dump from a file
 */
void do_cmd_load_screen(void)
{
	TERM_COLOR a = 0;
	SYMBOL_CODE c = ' ';
	bool okay = TRUE;
	FILE *fff;
	char buf[1024];
	TERM_LEN wid, hgt;
	Term_get_size(&wid, &hgt);
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");
	fff = my_fopen(buf, "r");
	if (!fff)
	{
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), buf);
		msg_print(NULL);
		return;
	}

	screen_save();
	Term_clear();
	for (TERM_LEN y = 0; okay; y++)
	{
		if (!fgets(buf, 1024, fff)) okay = FALSE;

		if (buf[0] == '\n' || buf[0] == '\0') break;
		if (y >= hgt) continue;

		for (TERM_LEN x = 0; x < wid - 1; x++)
		{
			if (buf[x] == '\n' || buf[x] == '\0') break;

			Term_draw(x, y, TERM_WHITE, buf[x]);
		}
	}

	for (TERM_LEN y = 0; okay; y++)
	{
		if (!fgets(buf, 1024, fff)) okay = FALSE;

		if (buf[0] == '\n' || buf[0] == '\0') break;
		if (y >= hgt) continue;

		for (TERM_LEN x = 0; x < wid - 1; x++)
		{
			if (buf[x] == '\n' || buf[x] == '\0') break;

			(void)(Term_what(x, y, &a, &c));
			for (int i = 0; i < 16; i++)
			{
				if (hack[i] == buf[x]) a = (byte)i;
			}

			Term_draw(x, y, a, c);
		}
	}

	my_fclose(fff);
	prt(_("ファイルに書き出された画面(記念撮影)をロードしました。", "Screen dump loaded."), 0, 0);
	flush();
	inkey();
	screen_load();
}
