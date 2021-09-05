/*!
 * @brief 記念撮影のセーブとロード
 * @date 2020/04/22
 * @Author Hourier
 */

#include "cmd-io/cmd-process-screen.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "view/display-messages.h"

// Encode the screen colors
static char hack[17] = "dwsorgbuDWvyRGBU";

static concptr tags[4] = { "HEADER_START:", "HEADER_END:", "FOOTER_START:", "FOOTER_END:", };
static concptr html_head[3] = { "<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n", "<pre>", 0, };
static concptr html_foot[3] = { "</pre>\n", "</body>\n</html>\n", 0, };

/*!
 * @brief 一時ファイルを読み込み、ファイルに書き出す
 * @param fff ファイルへの参照ポインタ
 * @param tempfff 一時ファイルへの参照ポインタ
 * @param buf バッファ
 * @param buf_size バッファサイズ
 * @param num_tag タグ番号
 * @todo io/ 以下に移したいところだが、このファイルの行数も大したことがないので一旦保留
 */
static void read_temporary_file(FILE *fff, FILE *tmpfff, char buf[], size_t buf_size, int num_tag)
{
	bool is_first_line = true;
	int next_tag = num_tag + 1;
	while (!angband_fgets(tmpfff, buf, buf_size))
	{
		if (is_first_line)
		{
			if (strncmp(buf, tags[num_tag], strlen(tags[num_tag])) == 0)
				is_first_line = false;

			continue;
		}

		if (strncmp(buf, tags[next_tag], strlen(tags[next_tag])) == 0)
			break;

		fprintf(fff, "%s\n", buf);
	}
}


/*!
 * @brief 記念撮影を1行ダンプする
 * @param wid 幅
 * @param y 現在の行位置
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void screen_dump_one_line(int wid, int y, FILE *fff)
{
	TERM_COLOR a = 0, old_a = 0;
	char c = ' ';
	for (TERM_LEN x = 0; x < wid - 1; x++)
	{
		concptr cc = nullptr;
		(void)(term_what(x, y, &a, &c));
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


/*!
 * @brief 記念撮影を行方向にスイープする
 * @param wid 幅
 * @param hgt 高さ
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void screen_dump_lines(int wid, int hgt, FILE *fff)
{
	for (TERM_LEN y = 0; y < hgt; y++)
	{
		if (y != 0)
			fprintf(fff, "\n");

		screen_dump_one_line(wid, y, fff);
	}
}


/*!
 * @brief ファイルへ書き込めない場合にエラーを表示する
 * @param fff ダンプファイルへの参照ポインタ
 * @param buf バッファ
 * @return ファイルへ書き込めるならTRUE、書き込めないならFALSE
 */
static bool check_screen_html_can_open(FILE *fff, char *filename, int message)
{
	if (fff) return true;
	if (message == 0) return false;

	msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename);
	msg_print(nullptr);
	return false;
}


/*!
 * @brief HTMLヘッダを書き込む
 * @param tmpfff 一時ファイルへの参照ポインタ
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param buf バッファ
 * @param buf_size バッファサイズ
 */
static void write_html_header(FILE *tmpfff, FILE *fff, char buf[], size_t buf_size)
{
	if (tmpfff)
	{
		read_temporary_file(fff, tmpfff, buf, buf_size, 0);
		return;
	}

	for (int i = 0; html_head[i]; i++)
		fputs(html_head[i], fff);
}


/*!
 * @brief HTMLフッタを書き込む
 * @param tmpfff 一時ファイルへの参照ポインタ
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param buf バッファ
 * @param buf_size バッファサイズ
 */
static void write_html_footer(FILE *tmpfff, FILE *fff, char buf[], size_t buf_size)
{
	fprintf(fff, "</font>");
	if (!tmpfff)
	{
		for (int i = 0; html_foot[i]; i++)
			fputs(html_foot[i], fff);
	}
	else
	{
		rewind(tmpfff);
		read_temporary_file(fff, tmpfff, buf, buf_size, 2);
		angband_fclose(tmpfff);
	}

	fprintf(fff, "\n");
}


void do_cmd_save_screen_html_aux(char *filename, int message)
{
	TERM_LEN wid, hgt;
	term_get_size(&wid, &hgt);
	FILE *fff;
	fff = angband_fopen(filename, "w");
	if (!check_screen_html_can_open(fff, filename, message)) return;

	if (message) screen_save();

	char buf[2048];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "htmldump.prf");
	FILE *tmpfff;
	tmpfff = angband_fopen(buf, "r");
	write_html_header(tmpfff, fff, buf, sizeof(buf));
	screen_dump_lines(wid, hgt, fff);
	write_html_footer(tmpfff, fff, buf, sizeof(buf));
	angband_fclose(fff);
	if (message)
	{
		msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
		msg_print(nullptr);
	}

	if (message)
		screen_load();
}


/*!
 * @brief HTML方式で記念撮影する / Save a screen dump to a file
 * @param なし
 */
static void do_cmd_save_screen_html(void)
{
	char buf[1024], tmp[256] = "screen.html";

	if (!get_string(_("ファイル名: ", "File name: "), tmp, 80))
		return;
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

	msg_print(nullptr);

	do_cmd_save_screen_html_aux(buf, 1);
}


/*!
 * @brief 記念撮影の方式を問い合わせる
 * @param html_dump HTMLダンプするか否か
 * @return ダンプするならTRUE、キャンセルならFALSE
 */
static bool ask_html_dump(bool *html_dump)
{
	while (true)
	{
		char c = inkey();
		if (c == 'Y' || c == 'y')
		{
			*html_dump = false;
			return true;
		}
		
		if (c == 'H' || c == 'h')
		{
			*html_dump = true;
			return true;
		}

		prt("", 0, 0);
		return false;
	}

	// コンパイル警告対応.
	return false;
}


/*!
 * @brief ファイルへ書き込めない場合にエラーを表示する
 * @param fff ダンプファイルへの参照ポインタ
 * @param buf バッファ
 * @return ファイルへ書き込めるならTRUE、書き込めないならFALSE
 */
static bool check_screen_text_can_open(FILE *fff, char buf[])
{
	if (fff) return true;

	msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
	msg_print(nullptr);
	return false;
}


/*!
 * @brief テキスト方式で記念撮影する
 * @param wid 幅
 * @param hgt 高さ
 * @return 記念撮影に成功したらTRUE、ファイルが開けなかったらFALSE
 * @todo どこかバグっていて、(恐らく初期化されていない)変な文字列まで出力される
 */
static bool do_cmd_save_screen_text(int wid, int hgt)
{
	TERM_COLOR a = 0;
	SYMBOL_CODE c = ' ';
	FILE *fff;
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");
	fff = angband_fopen(buf, "w");
	if (!check_screen_text_can_open(fff, buf)) return false;

	screen_save();
	for (TERM_LEN y = 0; y < hgt; y++)
	{
		TERM_LEN x;
		for (x = 0; x < wid - 1; x++)
		{
			(void)(term_what(x, y, &a, &c));
			buf[x] = c;
		}

		buf[x] = '\0';
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
	for (TERM_LEN y = 0; y < hgt; y++)
	{
		TERM_LEN x;
		for (x = 0; x < wid - 1; x++)
		{
			(void)(term_what(x, y, &a, &c));
			buf[x] = hack[a & 0x0F];
		}

		buf[x] = '\0';
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
	angband_fclose(fff);
	msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
	msg_print(nullptr);
	screen_load();
	return true;
}


/*!
 * @brief 記念撮影のためにグラフィック使用をOFFにする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 記念撮影直前のグラフィックオプション
 */
static bool update_use_graphics(player_type *creature_ptr)
{
	if (!use_graphics) return true;

	use_graphics = false;
	reset_visuals(creature_ptr);
	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
	handle_stuff(creature_ptr);
	return false;
}


/*
 * Save a screen dump to a file
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void do_cmd_save_screen(player_type *creature_ptr)
{
	prt(_("記念撮影しますか？ [(y)es/(h)tml/(n)o] ", "Save screen dump? [(y)es/(h)tml/(n)o] "), 0, 0);
	bool html_dump;
	if (!ask_html_dump(&html_dump)) return;

	int wid, hgt;
	term_get_size(&wid, &hgt);

	bool old_use_graphics = update_use_graphics(creature_ptr);

	if (html_dump)
	{
		do_cmd_save_screen_html();
		do_cmd_redraw(creature_ptr);
	}
	else if (!do_cmd_save_screen_text(wid, hgt))
	{
		return;
	}

	if (old_use_graphics) return;

	use_graphics = true;
	reset_visuals(creature_ptr);
	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
	handle_stuff(creature_ptr);
}


/*!
 * @brief 白文字だけ画面に描画する 
 * @param buf 描画用バッファ
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param wid 幅
 * @param hgt 高さ
 * @todo 目的は不明瞭
 * @return ファイルが読み込めなくなったらFALSEで抜ける
 */
static bool draw_white_characters(char buf[], FILE *fff, int wid, int hgt)
{
	bool okay = true;
	for (TERM_LEN y = 0; okay; y++)
	{
		if (!fgets(buf, 1024, fff)) okay = false;

		if (buf[0] == '\n' || buf[0] == '\0') break;
		if (y >= hgt) continue;

		for (TERM_LEN x = 0; x < wid - 1; x++)
		{
			if (buf[x] == '\n' || buf[x] == '\0') break;

			term_draw(x, y, TERM_WHITE, buf[x]);
		}
	}

	return okay;
}


/*!
 * @brief 白以外の文字を画面に描画する
 * @param buf 描画用バッファ
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param wid 幅
 * @param hgt 高さ
 * @param 白文字が途中で読み込めなくなっていたらTRUE
 * @todo 目的は不明瞭
 */
static void draw_colored_characters(char buf[], FILE *fff, int wid, int hgt, bool okay)
{
	TERM_COLOR a = TERM_DARK;
	SYMBOL_CODE c = ' ';
	for (TERM_LEN y = 0; okay; y++)
	{
		if (!fgets(buf, 1024, fff)) okay = false;

		if (buf[0] == '\n' || buf[0] == '\0') break;
		if (y >= hgt) continue;

		for (TERM_LEN x = 0; x < wid - 1; x++)
		{
			if (buf[x] == '\n' || buf[x] == '\0') break;

			(void)(term_what(x, y, &a, &c));
			for (int i = 0; i < 16; i++)
			{
				if (hack[i] == buf[x]) a = (byte)i;
			}

			term_draw(x, y, a, c);
		}
	}
}


/*
 * @brief Load a screen dump from a file
 * @param なし
 */
void do_cmd_load_screen(void)
{
	FILE *fff;
	char buf[1024];
	TERM_LEN wid, hgt;
	term_get_size(&wid, &hgt);
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");
	fff = angband_fopen(buf, "r");
	if (!fff)
	{
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), buf);
		msg_print(nullptr);
		return;
	}

	screen_save();
	term_clear();
	bool okay = draw_white_characters(buf, fff, wid, hgt);
	draw_colored_characters(buf, fff, wid, hgt, okay);

	angband_fclose(fff);
	prt(_("ファイルに書き出された画面(記念撮影)をロードしました。", "Screen dump loaded."), 0, 0);
	flush();
	inkey();
	screen_load();
}
