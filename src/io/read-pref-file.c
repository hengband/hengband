/*
 * @file cmd-dump.c
 * @brief プレイヤーのインターフェイスに関するコマンドの実装 / Interface commands
 * @date 2020/03/01
 * @author Mogami & Hourier
 * -Mogami-
 * remove_auto_dump(orig_file, mark)
 *     Remove the old automatic dump of type "mark".
 * auto_dump_printf(fmt, ...)
 *     Dump a formatted string using fprintf().
 * open_auto_dump(buf, mark)
 *     Open a file, remove old dump, and add new header.
 * close_auto_dump(void)
 *     Add a footer, and close the file.
 */

#include "angband.h"
#include "io/read-pref-file.h"
#include "io/interpret-pref-file.h"
#include "autopick/autopick.h"
#include "files.h" // 暫定。コールバック化して後で消す.
#include "world.h"

// todo コールバック関数に変更するので、いずれ消す.
#define PREF_TYPE_NORMAL   0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

char auto_dump_header[] = "# vvvvvvv== %s ==vvvvvvv";
char auto_dump_footer[] = "# ^^^^^^^== %s ==^^^^^^^";

// Mark strings for auto dump

// Variables for auto dump
static int auto_dump_line_num;

/*!
 * todo 関数名を変更する
 * @brief process_pref_fileのサブルーチン /
 * Open the "user pref file" and parse it.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @param preftype prefファイルのタイプ
 * @return エラーコード
 */
static errr process_pref_file_aux(player_type *creature_ptr, concptr name, int preftype)
{
	FILE *fp;
	fp = my_fopen(name, "r");
	if (!fp) return -1;

	char buf[1024];
	char old[1024];
	int line = -1;
	errr err = 0;
	bool bypass = FALSE;
	while (my_fgets(fp, buf, sizeof(buf)) == 0)
	{
		line++;
		if (!buf[0]) continue;

#ifdef JP
		if (!iskanji(buf[0]))
#endif
			if (iswspace(buf[0])) continue;

		if (buf[0] == '#') continue;
		strcpy(old, buf);

		/* Process "?:<expr>" */
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
			char f;
			char *s;
			s = buf + 2;
			concptr v = process_pref_file_expr(creature_ptr, &s, &f);
			bypass = streq(v, "0");
			continue;
		}

		if (bypass) continue;

		/* Process "%:<file>" */
		if (buf[0] == '%')
		{
			static int depth_count = 0;
			if (depth_count > 20) continue;

			depth_count++;
			switch (preftype)
			{
			case PREF_TYPE_AUTOPICK:
				(void)process_autopick_file(creature_ptr, buf + 2);
				break;
			case PREF_TYPE_HISTPREF:
				(void)process_histpref_file(creature_ptr, buf + 2);
				break;
			default:
				(void)process_pref_file(creature_ptr, buf + 2);
				break;
			}

			depth_count--;
			continue;
		}

		err = interpret_pref_file(creature_ptr, buf);
		if (err != 0)
		{
			if (preftype != PREF_TYPE_AUTOPICK)
				break;
			
			process_autopick_file_command(buf);
			err = 0;
		}
	}

	if (err != 0)
	{
		/* Print error message */
		/* ToDo: Add better error messages */
		msg_format(_("ファイル'%s'の%d行でエラー番号%dのエラー。", "Error %d in line %d of file '%s'."),
			_(name, err), line, _(err, name));
		msg_format(_("('%s'を解析中)", "Parsing '%s'"), old);
		msg_print(NULL);
	}

	my_fclose(fp);
	return (err);
}


/*!
 * @brief pref設定ファイルを読み込み設定を反映させる /
 * Process the "user pref file" with the given name
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @return エラーコード
 * @details
 * <pre>
 * See the functions above for a list of legal "commands".
 * We also accept the special "?" and "%" directives, which
 * allow conditional evaluation and filename inclusion.
 * </pre>
 */
errr process_pref_file(player_type *creature_ptr, concptr name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, name);

	errr err1 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL);
	if (err1 > 0) return err1;

	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	errr err2 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL);
	if (err2 < 0 && !err1)
		return -2;

	return err2;
}


/*!
 * @brief 自動拾いファイルを読み込む /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name ファイル名
 * @details
 */
errr process_autopick_file(player_type *creature_ptr, concptr name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_AUTOPICK);
	return err;
}


/*!
 * @brief プレイヤーの生い立ちファイルを読み込む /
 * Process file for player's history editor.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name ファイル名
 * @return エラーコード
 * @details
 */
errr process_histpref_file(player_type *creature_ptr, concptr name)
{
	bool old_character_xtra = current_world_ptr->character_xtra;
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* Hack -- prevent modification birth options in this file */
	current_world_ptr->character_xtra = TRUE;
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_HISTPREF);
	current_world_ptr->character_xtra = old_character_xtra;
	return err;
}


/*!
 * @brief prf出力内容を消去する /
 * Remove old lines automatically generated before.
 * @param orig_file 消去を行うファイル名
 */
static void remove_auto_dump(concptr orig_file, concptr auto_dump_mark)
{
	char header_mark_str[80];
	char footer_mark_str[80];
	sprintf(header_mark_str, auto_dump_header, auto_dump_mark);
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);
	size_t mark_len = strlen(footer_mark_str);
	FILE *orig_fff;
	orig_fff = my_fopen(orig_file, "r");
	if (!orig_fff) return;

	char tmp_file[1024];
	FILE *tmp_fff;
	tmp_fff = my_fopen_temp(tmp_file, 1024);
	if (!tmp_fff)
	{
		msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), tmp_file);
		msg_print(NULL);
		return;
	}

	char buf[1024];
	bool between_mark = FALSE;
	bool changed = FALSE;
	int line_num = 0;
	long header_location = 0;
	while (TRUE)
	{
		if (my_fgets(orig_fff, buf, sizeof(buf)))
		{
			if (between_mark)
			{
				fseek(orig_fff, header_location, SEEK_SET);
				between_mark = FALSE;
				continue;
			}
			else
			{
				break;
			}
		}

		if (!between_mark)
		{
			if (!strcmp(buf, header_mark_str))
			{
				header_location = ftell(orig_fff);
				line_num = 0;
				between_mark = TRUE;
				changed = TRUE;
			}
			else
			{
				fprintf(tmp_fff, "%s\n", buf);
			}

			continue;
		}

		if (!strncmp(buf, footer_mark_str, mark_len))
		{
			int tmp;
			if (!sscanf(buf + mark_len, " (%d)", &tmp)
				|| tmp != line_num)
			{
				fseek(orig_fff, header_location, SEEK_SET);
			}

			between_mark = FALSE;
			continue;
		}

		line_num++;
	}

	my_fclose(orig_fff);
	my_fclose(tmp_fff);

	if (changed)
	{
		tmp_fff = my_fopen(tmp_file, "r");
		orig_fff = my_fopen(orig_file, "w");
		while (!my_fgets(tmp_fff, buf, sizeof(buf)))
			fprintf(orig_fff, "%s\n", buf);

		my_fclose(orig_fff);
		my_fclose(tmp_fff);
	}

	fd_kill(tmp_file);
}


/*!
 * @brief prfファイルのフォーマットに従った内容を出力する /
 * Dump a formatted line, using "vstrnfmt()".
 * @param fmt 出力内容
 */
void auto_dump_printf(FILE *auto_dump_stream, concptr fmt, ...)
{
	va_list vp;
	char buf[1024];
	va_start(vp, fmt);
	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);
	va_end(vp);
	for (concptr p = buf; *p; p++)
	{
		if (*p == '\n') auto_dump_line_num++;
	}

	fprintf(auto_dump_stream, "%s", buf);
}


/*!
 * @brief prfファイルをファイルオープンする /
 * Open file to append auto dump.
 * @param buf ファイル名
 * @param mark 出力するヘッダマーク
 * @return ファイルポインタを取得できたらTRUEを返す
 */
bool open_auto_dump(FILE **fpp, concptr buf, concptr mark)
{
	char header_mark_str[80];
	concptr auto_dump_mark = mark;
	sprintf(header_mark_str, auto_dump_header, auto_dump_mark);
	remove_auto_dump(buf, mark);
	*fpp = my_fopen(buf, "a");
	if (!fpp)
	{
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), buf);
		msg_print(NULL);
		return FALSE;
	}

	fprintf(*fpp, "%s\n", header_mark_str);
	auto_dump_line_num = 0;
	auto_dump_printf(*fpp, _("# *警告!!* 以降の行は自動生成されたものです。\n",
		"# *Warning!*  The lines below are an automatic dump.\n"));
	auto_dump_printf(*fpp, _("# *警告!!* 後で自動的に削除されるので編集しないでください。\n",
		"# Don't edit them; changes will be deleted and replaced automatically.\n"));
	return TRUE;
}

/*!
 * @brief prfファイルをファイルクローズする /
 * Append foot part and close auto dump.
 * @return なし
 */
void close_auto_dump(FILE **fpp, concptr auto_dump_mark)
{
	char footer_mark_str[80];
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);
	auto_dump_printf(*fpp, _("# *警告!!* 以降の行は自動生成されたものです。\n",
		"# *Warning!*  The lines below are an automatic dump.\n"));
	auto_dump_printf(*fpp, _("# *警告!!* 後で自動的に削除されるので編集しないでください。\n",
		"# Don't edit them; changes will be deleted and replaced automatically.\n"));
	fprintf(*fpp, "%s (%d)\n", footer_mark_str, auto_dump_line_num);
	my_fclose(*fpp);
}
