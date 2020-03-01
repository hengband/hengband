/*!
 * @file files.c
 * @brief ファイル入出力管理 / Purpose: code dealing with files (and death)
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 */

#include "angband.h"
#include "term.h"
#include "signal-handlers.h"
#include "uid-checker.h"
#include "files.h"
#include "core.h" // リファクタリングして後で消す

#include "birth.h"
#include "character-dump.h"
#include "cmd-dump.h"
#include "world.h"
#include "player-move.h"
#include "player-personality.h"
#include "player-effects.h"
#include "monster-status.h"
#include "view-mainwindow.h"
#include "objectkind.h"
#include "autopick.h"
#include "save.h"
#include "io/tokenizer.h"
#include "io/process-pref-file.h" // 暫定。依存性の向きがこれで良いか要確認.

#define PREF_TYPE_NORMAL   0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

concptr ANGBAND_DIR; //!< Path name: The main "lib" directory This variable is not actually used anywhere in the code
concptr ANGBAND_DIR_APEX; //!< High score files (binary) These files may be portable between platforms
concptr ANGBAND_DIR_BONE; //!< Bone files for player ghosts (ascii) These files are portable between platforms
concptr ANGBAND_DIR_DATA; //!< Binary image files for the "*_info" arrays (binary) These files are not portable between platforms
concptr ANGBAND_DIR_EDIT; //!< Textual template files for the "*_info" arrays (ascii) These files are portable between platforms
concptr ANGBAND_DIR_SCRIPT; //!< Script files These files are portable between platforms.
concptr ANGBAND_DIR_FILE; //!< Various extra files (ascii) These files may be portable between platforms
concptr ANGBAND_DIR_HELP; //!< Help files (normal) for the online help (ascii) These files are portable between platforms
concptr ANGBAND_DIR_INFO; //!< Help files (spoilers) for the online help (ascii) These files are portable between platforms
concptr ANGBAND_DIR_PREF; //!< Default user "preference" files (ascii) These files are rarely portable between platforms
concptr ANGBAND_DIR_SAVE; //!< Savefiles for current characters (binary)
concptr ANGBAND_DIR_USER; //!< User "preference" files (ascii) These files are rarely portable between platforms
concptr ANGBAND_DIR_XTRA; //!< Various extra files (binary) These files are rarely portable between platforms

/*
 * Buffer to hold the current savefile name
 * 'savefile' holds full path name. 'savefile_base' holds only base name.
 */
char savefile[1024];
char savefile_base[40];

/*!
 * todo サブルーチンとは言い難い。autopick.c から直接呼ばれている
 * @brief process_pref_fileのサブルーチンとして条件分岐処理の解釈と結果を返す
 * Helper function for "process_pref_file()"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param sp テキスト文字列の参照ポインタ
 * @param fp 再帰中のポインタ参照
 * @return
 * @details
 * <pre>
 * Input:
 *   v: output buffer array
 *   f: final character
 * Output:
 *   result
 * </pre>
 */
concptr process_pref_file_expr(player_type *creature_ptr, char **sp, char *fp)
{
	char *s;
	s = (*sp);
	while (iswspace(*s)) s++;

	char *b;
	b = s;

	concptr v = "?o?o?";

	char b1 = '[';
	char b2 = ']';
	char f = ' ';
	static char tmp[16];
	if (*s == b1)
	{
		concptr p;
		concptr t;

		/* Skip b1 */
		s++;

		/* First */
		t = process_pref_file_expr(creature_ptr, &s, &f);

		if (!*t)
		{
		}
		else if (streq(t, "IOR"))
		{
			v = "0";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}
		else if (streq(t, "EQU"))
		{
			v = "0";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = process_pref_file_expr(creature_ptr, &s, &f);
				if (streq(t, p)) v = "1";
			}
		}
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && atoi(p) > atoi(t)) v = "0";
			}
		}
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && atoi(p) < atoi(t)) v = "0";
			}
		}
		else
		{
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
		}

		if (f != b2) v = "?x?x?";

		if ((f = *s) != '\0') *s++ = '\0';

		*fp = f;
		*sp = s;
		return v;
	}

	/* Accept all printables except spaces and brackets */
#ifdef JP
	while (iskanji(*s) || (isprint(*s) && !my_strchr(" []", *s)))
	{
		if (iskanji(*s)) s++;
		s++;
	}
#else
	while (isprint(*s) && !my_strchr(" []", *s)) ++s;
#endif

	if ((f = *s) != '\0') *s++ = '\0';

	if (*b != '$')
	{
		v = b;
		*fp = f;
		*sp = s;
		return v;
	}

	if (streq(b + 1, "SYS"))
	{
		v = ANGBAND_SYS;
	}
	else if (streq(b + 1, "KEYBOARD"))
	{
		v = ANGBAND_KEYBOARD;
	}
	else if (streq(b + 1, "GRAF"))
	{
		v = ANGBAND_GRAF;
	}
	else if (streq(b + 1, "MONOCHROME"))
	{
		if (arg_monochrome)
			v = "ON";
		else
			v = "OFF";
	}
	else if (streq(b + 1, "RACE"))
	{
#ifdef JP
		v = rp_ptr->E_title;
#else
		v = rp_ptr->title;
#endif
	}
	else if (streq(b + 1, "CLASS"))
	{
#ifdef JP
		v = cp_ptr->E_title;
#else
		v = cp_ptr->title;
#endif
	}
	else if (streq(b + 1, "PLAYER"))
	{
		static char tmp_player_name[32];
		char *pn, *tpn;
		for (pn = creature_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++)
		{
#ifdef JP
			if (iskanji(*pn))
			{
				*(tpn++) = *(pn++);
				*tpn = *pn;
				continue;
			}
#endif
			*tpn = my_strchr(" []", *pn) ? '_' : *pn;
		}

		*tpn = '\0';
		v = tmp_player_name;
	}
	else if (streq(b + 1, "REALM1"))
	{
#ifdef JP
		v = E_realm_names[creature_ptr->realm1];
#else
		v = realm_names[creature_ptr->realm1];
#endif
	}
	else if (streq(b + 1, "REALM2"))
	{
#ifdef JP
		v = E_realm_names[creature_ptr->realm2];
#else
		v = realm_names[creature_ptr->realm2];
#endif
	}
	else if (streq(b + 1, "LEVEL"))
	{
		sprintf(tmp, "%02d", creature_ptr->lev);
		v = tmp;
	}
	else if (streq(b + 1, "AUTOREGISTER"))
	{
		if (creature_ptr->autopick_autoregister)
			v = "1";
		else
			v = "0";
	}
	else if (streq(b + 1, "MONEY"))
	{
		sprintf(tmp, "%09ld", (long int)creature_ptr->au);
		v = tmp;
	}

	*fp = f;
	*sp = s;
	return v;
}


/*!
 * @brief process_pref_fileのサブルーチン /
 * Open the "user pref file" and parse it.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @param preftype prefファイルのタイプ
 * @return エラーコード
 * @details
 * <pre>
 * Input:
 *   v: output buffer array
 *   f: final character
 * Output:
 *   result
 * </pre>
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

		err = process_pref_file_command(creature_ptr, buf);
		if (err != 0)
		{
			if (preftype != PREF_TYPE_AUTOPICK)
				break;
			err = process_autopick_file_command(buf);
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
 * @brief プレイヤーステータスをファイルダンプ出力する
 * Hack -- Dump a character description file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 出力ファイル名
 * @return エラーコード
 * @details
 * Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
errr file_character(player_type *creature_ptr, concptr name, display_player_pf display_player, map_name_pf map_name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	FILE_TYPE(FILE_TYPE_TEXT);

	int	fd = fd_open(buf, O_RDONLY);
	if (fd >= 0)
	{
		char out_val[160];
		(void)fd_close(fd);
		(void)sprintf(out_val, _("現存するファイル %s に上書きしますか? ", "Replace existing file %s? "), buf);
		if (get_check_strict(out_val, CHECK_NO_HISTORY)) fd = -1;
	}

	FILE *fff = NULL;
	if (fd < 0) fff = my_fopen(buf, "w");

	if (!fff)
	{
		prt(_("キャラクタ情報のファイルへの書き出しに失敗しました！", "Character dump failed!"), 0, 0);
		(void)inkey();
		return -1;
	}

	make_character_dump(creature_ptr, fff, update_playtime, display_player, map_name);
	my_fclose(fff);
	msg_print(_("キャラクタ情報のファイルへの書き出しに成功しました。", "Character dump successful."));
	msg_print(NULL);
	return 0;
}


/*!
 * @brief ファイル内容の一行をコンソールに出力する
 * Display single line of on-line help file
 * @param str 出力する文字列
 * @param cy コンソールの行
 * @param shower 確認中
 * @return なし
 * @details
 * <pre>
 * You can insert some special color tag to change text color.
 * Such as...
 * WHITETEXT [[[[y|SOME TEXT WHICH IS DISPLAYED IN YELLOW| WHITETEXT
 * A colored segment is between "[[[[y|" and the last "|".
 * You can use any single character in place of the "|".
 * </pre>
 */
static void show_file_aux_line(concptr str, int cy, concptr shower)
{
	char lcstr[1024];
	if (shower)
	{
		strcpy(lcstr, str);
		str_tolower(lcstr);
	}

	int cx = 0;
	Term_gotoxy(cx, cy);

	static const char tag_str[] = "[[[[";
	byte color = TERM_WHITE;
	char in_tag = '\0';
	for (int i = 0; str[i];)
	{
		int len = strlen(&str[i]);
		int showercol = len + 1;
		int bracketcol = len + 1;
		int endcol = len;
		concptr ptr;
		if (shower)
		{
			ptr = my_strstr(&lcstr[i], shower);
			if (ptr) showercol = ptr - &lcstr[i];
		}

		ptr = in_tag ? my_strchr(&str[i], in_tag) : my_strstr(&str[i], tag_str);
		if (ptr) bracketcol = ptr - &str[i];
		if (bracketcol < endcol) endcol = bracketcol;
		if (showercol < endcol) endcol = showercol;

		Term_addstr(endcol, color, &str[i]);
		cx += endcol;
		i += endcol;

		if (endcol == showercol)
		{
			int showerlen = strlen(shower);
			Term_addstr(showerlen, TERM_YELLOW, &str[i]);
			cx += showerlen;
			i += showerlen;
			continue;
		}

		if (endcol != bracketcol) continue;

		if (in_tag)
		{
			i++;
			in_tag = '\0';
			color = TERM_WHITE;
			continue;
		}

		i += sizeof(tag_str) - 1;
		color = color_char_to_attr(str[i]);
		if (color == 255 || str[i + 1] == '\0')
		{
			color = TERM_WHITE;
			Term_addstr(-1, TERM_WHITE, tag_str);
			cx += sizeof(tag_str) - 1;
			continue;
		}

		i++;
		in_tag = str[i];
		i++;
	}

	Term_erase(cx, cy, 255);
}


/*!
 * @brief ファイル内容をコンソールに出力する
 * Recursive file perusal.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param show_version TRUEならばコンソール上にゲームのバージョンを表示する
 * @param name ファイル名の文字列
 * @param what 内容キャプションの文字列
 * @param line 表示の現在行
 * @param mode オプション
 * @return なし
 * @details
 * <pre>
 * Process various special text in the input file, including
 * the "menu" structures used by the "help file" system.
 * Return FALSE on 'q' to exit from a deep, otherwise TRUE.
 * </pre>
 */
bool show_file(player_type *creature_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode)
{
	int wid, hgt;
	Term_get_size(&wid, &hgt);

	char finder_str[81];
	strcpy(finder_str, "");

	char shower_str[81];
	strcpy(shower_str, "");

	char caption[128];
	strcpy(caption, "");

	char hook[68][32];
	for (int i = 0; i < 68; i++)
	{
		hook[i][0] = '\0';
	}

	char filename[1024];
	strcpy(filename, name);
	int n = strlen(filename);

	concptr tag = NULL;
	for (int i = 0; i < n; i++)
	{
		if (filename[i] == '#')
		{
			filename[i] = '\0';
			tag = filename + i + 1;
			break;
		}
	}

	name = filename;
	FILE *fff = NULL;
	char path[1024];
	if (what)
	{
		strcpy(caption, what);
		strcpy(path, name);
		fff = my_fopen(path, "r");
	}

	if (!fff)
	{
		sprintf(caption, _("ヘルプ・ファイル'%s'", "Help file '%s'"), name);
		path_build(path, sizeof(path), ANGBAND_DIR_HELP, name);
		fff = my_fopen(path, "r");
	}

	if (!fff)
	{
		sprintf(caption, _("スポイラー・ファイル'%s'", "Info file '%s'"), name);
		path_build(path, sizeof(path), ANGBAND_DIR_INFO, name);
		fff = my_fopen(path, "r");
	}

	if (!fff)
	{
		path_build(path, sizeof(path), ANGBAND_DIR, name);

		for (int i = 0; path[i]; i++)
			if ('\\' == path[i])
				path[i] = PATH_SEP[0];

		sprintf(caption, _("スポイラー・ファイル'%s'", "Info file '%s'"), name);
		fff = my_fopen(path, "r");
	}

	if (!fff)
	{
		msg_format(_("'%s'をオープンできません。", "Cannot open '%s'."), name);
		msg_print(NULL);

		return TRUE;
	}

	int skey;
	int next = 0;
	int size = 0;
	int back = 0;
	bool menu = FALSE;
	char buf[1024];
	bool reverse = (line < 0);
	while (TRUE)
	{
		char *str = buf;
		if (my_fgets(fff, buf, sizeof(buf))) break;
		if (!prefix(str, "***** "))
		{
			next++;
			continue;
		}

		if ((str[6] == '[') && isalpha(str[7]))
		{
			int k = str[7] - 'A';
			menu = TRUE;
			if ((str[8] == ']') && (str[9] == ' '))
			{
				strncpy(hook[k], str + 10, 31);
				hook[k][31] = '\0';
			}

			continue;
		}

		if (str[6] != '<') continue;

		size_t len = strlen(str);
		if (str[len - 1] == '>')
		{
			str[len - 1] = '\0';
			if (tag && streq(str + 7, tag)) line = next;
		}
	}

	size = next;
	int rows = hgt - 4;
	if (line == -1)
		line = ((size - 1) / rows)*rows;

	Term_clear();

	concptr find = NULL;
	while (TRUE)
	{
		if (line >= size - rows)
			line = size - rows;
		if (line < 0) line = 0;

		if (next > line)
		{
			my_fclose(fff);
			fff = my_fopen(path, "r");
			if (!fff) return FALSE;

			next = 0;
		}

		while (next < line)
		{
			if (my_fgets(fff, buf, sizeof(buf))) break;
			if (prefix(buf, "***** ")) continue;
			next++;
		}

		int row_count = 0;
		concptr shower = NULL;
		for (int i = 0; i < rows; i++)
		{
			concptr str = buf;
			if (!i) line = next;
			if (my_fgets(fff, buf, sizeof(buf))) break;
			if (prefix(buf, "***** ")) continue;
			next++;
			if (find && !i)
			{
				char lc_buf[1024];
				strcpy(lc_buf, str);
				str_tolower(lc_buf);
				if (!my_strstr(lc_buf, find)) continue;
			}

			find = NULL;
			show_file_aux_line(str, i + 2, shower);
			row_count++;
		}

		while (row_count < rows)
		{
			Term_erase(0, row_count + 2, 255);
			row_count++;
		}

		if (find)
		{
			bell();
			line = back;
			find = NULL;
			continue;
		}

		if (show_version)
		{
			prt(format(_("[変愚蛮怒 %d.%d.%d, %s, %d/%d]", "[Hengband %d.%d.%d, %s, Line %d/%d]"),
				FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH,
				caption, line, size), 0, 0);
		}
		else
		{
			prt(format(_("[%s, %d/%d]", "[%s, Line %d/%d]"),
				caption, line, size), 0, 0);
		}

		if (size <= rows)
		{
			prt(_("[キー:(?)ヘルプ (ESC)終了]", "[Press ESC to exit.]"), hgt - 1, 0);
		}
		else
		{
#ifdef JP
			if (reverse)
				prt("[キー:(RET/スペース)↑ (-)↓ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
			else
				prt("[キー:(RET/スペース)↓ (-)↑ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
#else
			prt("[Press Return, Space, -, =, /, |, or ESC to exit.]", hgt - 1, 0);
#endif
		}

		skey = inkey_special(TRUE);
		switch (skey)
		{
		case '?':
			if (strcmp(name, _("jhelpinfo.txt", "helpinfo.txt")) != 0)
				show_file(creature_ptr, TRUE, _("jhelpinfo.txt", "helpinfo.txt"), NULL, 0, mode);
			break;
		case '=':
			prt(_("強調: ", "Show: "), hgt - 1, 0);

			char back_str[81];
			strcpy(back_str, shower_str);
			if (askfor(shower_str, 80))
			{
				if (shower_str[0])
				{
					str_tolower(shower_str);
					shower = shower_str;
				}
				else shower = NULL;
			}
			else strcpy(shower_str, back_str);
			break;

		case '/':
		case KTRL('s'):
			prt(_("検索: ", "Find: "), hgt - 1, 0);
			strcpy(back_str, finder_str);
			if (askfor(finder_str, 80))
			{
				if (finder_str[0])
				{
					find = finder_str;
					back = line;
					line = line + 1;
					str_tolower(finder_str);
					shower = finder_str;
				}
				else shower = NULL;
			}
			else strcpy(finder_str, back_str);
			break;

		case '#':
		{
			char tmp[81];
			prt(_("行: ", "Goto Line: "), hgt - 1, 0);
			strcpy(tmp, "0");

			if (askfor(tmp, 80)) line = atoi(tmp);
			break;
		}

		case SKEY_TOP:
			line = 0;
			break;

		case SKEY_BOTTOM:
			line = ((size - 1) / rows) * rows;
			break;

		case '%':
		{
			char tmp[81];
			prt(_("ファイル・ネーム: ", "Goto File: "), hgt - 1, 0);
			strcpy(tmp, _("jhelp.hlp", "help.hlp"));

			if (askfor(tmp, 80))
			{
				if (!show_file(creature_ptr, TRUE, tmp, NULL, 0, mode)) skey = 'q';
			}

			break;
		}

		case '-':
			line = line + (reverse ? rows : -rows);
			if (line < 0) line = 0;
			break;

		case SKEY_PGUP:
			line = line - rows;
			if (line < 0) line = 0;
			break;

		case '\n':
		case '\r':
			line = line + (reverse ? -1 : 1);
			if (line < 0) line = 0;
			break;

		case '8':
		case SKEY_UP:
			line--;
			if (line < 0) line = 0;
			break;

		case '2':
		case SKEY_DOWN:
			line++;
			break;

		case ' ':
			line = line + (reverse ? -rows : rows);
			if (line < 0) line = 0;
			break;

		case SKEY_PGDOWN:
			line = line + rows;
			break;
		}

		if (menu)
		{
			int key = -1;
			if (!(skey & SKEY_MASK) && isalpha(skey))
				key = skey - 'A';

			if ((key > -1) && hook[key][0])
			{
				/* Recurse on that file */
				if (!show_file(creature_ptr, TRUE, hook[key], NULL, 0, mode))
					skey = 'q';
			}
		}

		if (skey == '|')
		{
			FILE *ffp;
			char buff[1024];
			char xtmp[82];

			strcpy(xtmp, "");

			if (!get_string(_("ファイル名: ", "File name: "), xtmp, 80)) continue;
			my_fclose(fff);
			path_build(buff, sizeof(buff), ANGBAND_DIR_USER, xtmp);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			ffp = my_fopen(buff, "w");

			if (!(fff && ffp))
			{
				msg_print(_("ファイルを開けません。", "Failed to open file."));
				skey = ESCAPE;
				break;
			}

			sprintf(xtmp, "%s: %s", creature_ptr->name, what ? what : caption);
			my_fputs(ffp, xtmp, 80);
			my_fputs(ffp, "\n", 80);

			while (!my_fgets(fff, buff, sizeof(buff)))
				my_fputs(ffp, buff, 80);
			my_fclose(fff);
			my_fclose(ffp);
			fff = my_fopen(path, "r");
		}

		if ((skey == ESCAPE) || (skey == '<')) break;

		if (skey == KTRL('q')) skey = 'q';

		if (skey == 'q') break;
	}

	my_fclose(fff);
	return (skey != 'q');
}


/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @return なし
 * @details
 */
void do_cmd_save_game(player_type *creature_ptr, int is_autosave)
{
	if (is_autosave)
	{
		msg_print(_("自動セーブ中", "Autosaving the game..."));
	}
	else
	{
		disturb(creature_ptr, TRUE, TRUE);
	}

	msg_print(NULL);
	handle_stuff(creature_ptr);
	prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);
	Term_fresh();
	(void)strcpy(creature_ptr->died_from, _("(セーブ)", "(saved)"));
	signals_ignore_tstp();
	if (save_player(creature_ptr))
	{
		prt(_("ゲームをセーブしています... 終了", "Saving game... done."), 0, 0);
	}
	else
	{
		prt(_("ゲームをセーブしています... 失敗！", "Saving game... failed!"), 0, 0);
	}

	signals_handle_tstp();
	Term_fresh();
	(void)strcpy(creature_ptr->died_from, _("(元気に生きている)", "(alive and well)"));
	current_world_ptr->is_loading_now = FALSE;
	update_creature(creature_ptr);
	mproc_init(creature_ptr->current_floor_ptr);
	current_world_ptr->is_loading_now = TRUE;
}


/*!
 * @brief セーブ後にゲーム中断フラグを立てる/
 * Save the game and exit
 * @return なし
 * @details
 */
void do_cmd_save_and_exit(player_type *creature_ptr)
{
	creature_ptr->playing = FALSE;
	creature_ptr->leaving = TRUE;
	exe_write_diary(creature_ptr, DIARY_GAMESTART, 0, _("----ゲーム中断----", "---- Save and Exit Game ----"));
}


/*!
 * @brief 異常発生時のゲーム緊急終了処理 /
 * Handle abrupt death of the visual system
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 */
void exit_game_panic(player_type *creature_ptr)
{
	if (!current_world_ptr->character_generated || current_world_ptr->character_saved)
		quit(_("緊急事態", "panic"));
	msg_flag = FALSE;

	prt("", 0, 0);
	disturb(creature_ptr, TRUE, TRUE);
	if (creature_ptr->chp < 0) creature_ptr->is_dead = FALSE;

	creature_ptr->panic_save = 1;
	signals_ignore_tstp();
	(void)strcpy(creature_ptr->died_from, _("(緊急セーブ)", "(panic save)"));
	if (!save_player(creature_ptr)) quit(_("緊急セーブ失敗！", "panic save failed!"));
	quit(_("緊急セーブ成功！", "panic save succeeded!"));
}


/*!
 * @brief ファイルからランダムに行を一つ取得する /
 * Get a random line from a file
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @param output 出力先の文字列参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Based on the monster speech patch by Matt Graham,
 * </pre>
 */
errr get_rnd_line(concptr file_name, int entry, char *output)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, file_name);
	FILE *fp;
	fp = my_fopen(buf, "r");
	if (!fp) return -1;

	int test;
	int line_num = 0;
	while (TRUE)
	{
		if (my_fgets(fp, buf, sizeof(buf)) != 0)
		{
			my_fclose(fp);
			return -1;
		}

		line_num++;
		if ((buf[0] != 'N') || (buf[1] != ':')) continue;

		if (buf[2] == '*')
		{
			break;
		}
		else if (buf[2] == 'M')
		{
			if (r_info[entry].flags1 & RF1_MALE) break;
		}
		else if (buf[2] == 'F')
		{
			if (r_info[entry].flags1 & RF1_FEMALE) break;
		}
		else if (sscanf(&(buf[2]), "%d", &test) != EOF)
		{
			if (test == entry) break;
		}

		msg_format("Error in line %d of %s!", line_num, file_name);
		my_fclose(fp);
		return -1;
	}

	int counter;
	for (counter = 0; ; counter++)
	{
		while (TRUE)
		{
			test = my_fgets(fp, buf, sizeof(buf));
			if (!test)
			{
				/* Ignore lines starting with 'N:' */
				if ((buf[0] == 'N') && (buf[1] == ':')) continue;

				if (buf[0] != '#') break;
			}
			else break;
		}

		if (!buf[0]) break;

		if (one_in_(counter + 1)) strcpy(output, buf);
	}

	my_fclose(fp);
	return counter ? 0 : -1;
}


#ifdef JP
/*!
 * @brief ファイルからランダムに行を一つ取得する(日本語文字列のみ) /
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @param output 出力先の文字列参照ポインタ
 * @param count 試行回数
 * @return エラーコード
 * @details
 */
errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count)
{
	errr result = 1;
	for (int i = 0; i < count; i++)
	{
		result = get_rnd_line(file_name, entry, output);
		if (result) break;
		bool kanji = FALSE;
		for (int j = 0; output[j]; j++) kanji |= iskanji(output[j]);
		if (kanji) break;
	}

	return result;
}
#endif


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
 * @brief ファイル位置をシーク /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fd ファイルディスクリプタ
 * @param where ファイルバイト位置
 * @param flag FALSEならば現ファイルを超えた位置へシーク時エラー、TRUEなら足りない間を0で埋め尽くす
 * @return エラーコード
 * @details
 */
static errr counts_seek(player_type *creature_ptr, int fd, u32b where, bool flag)
{
	char temp1[128], temp2[128];
#ifdef SAVEFILE_USE_UID
	(void)sprintf(temp1, "%d.%s.%d%d%d", creature_ptr->player_uid, savefile_base, creature_ptr->pclass, creature_ptr->pseikaku, creature_ptr->age);
#else
	(void)sprintf(temp1, "%s.%d%d%d", savefile_base, creature_ptr->pclass, creature_ptr->pseikaku, creature_ptr->age);
#endif
	for (int i = 0; temp1[i]; i++)
		temp1[i] ^= (i + 1) * 63;

	int seekpoint = 0;
	u32b zero_header[3] = { 0L, 0L, 0L };
	while (TRUE)
	{
		if (fd_seek(fd, seekpoint + 3 * sizeof(u32b)))
			return 1;
		if (fd_read(fd, (char*)(temp2), sizeof(temp2)))
		{
			if (!flag)
				return 1;
			/* add new name */
			fd_seek(fd, seekpoint);
			fd_write(fd, (char*)zero_header, 3 * sizeof(u32b));
			fd_write(fd, (char*)(temp1), sizeof(temp1));
			break;
		}

		if (strcmp(temp1, temp2) == 0)
			break;

		seekpoint += 128 + 3 * sizeof(u32b);
	}

	return fd_seek(fd, seekpoint + where * sizeof(u32b));
}


/*!
 * @brief ファイル位置を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @return エラーコード
 * @details
 */
u32b counts_read(player_type *creature_ptr, int where)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));
	int fd = fd_open(buf, O_RDONLY);

	u32b count = 0;
	if (counts_seek(creature_ptr, fd, where, FALSE) ||
		fd_read(fd, (char*)(&count), sizeof(u32b)))
		count = 0;

	(void)fd_close(fd);

	return count;
}


/*!
 * @brief ファイル位置に書き込む /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @param count 書き込む値
 * @return エラーコード
 * @details
 */
errr counts_write(player_type *creature_ptr, int where, u32b count)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));

	safe_setuid_grab();
	int fd = fd_open(buf, O_RDWR);
	safe_setuid_drop();
	if (fd < 0)
	{
		FILE_TYPE(FILE_TYPE_DATA);
		safe_setuid_grab();
		fd = fd_make(buf, 0644);
		safe_setuid_drop();
	}

	safe_setuid_grab();
	errr err = fd_lock(fd, F_WRLCK);
	safe_setuid_drop();
	if (err) return 1;

	counts_seek(creature_ptr, fd, where, TRUE);
	fd_write(fd, (char*)(&count), sizeof(u32b));
	safe_setuid_grab();
	err = fd_lock(fd, F_UNLCK);
	safe_setuid_drop();

	if (err) return 1;

	(void)fd_close(fd);
	return 0;
}


/*!
 * @brief 墓のアスキーアートテンプレを読み込む
 * @param buf テンプレへのバッファ
 * @param buf_size バッファの長さ
 * @return なし
 */
void read_dead_file(char *buf, size_t buf_size)
{
	path_build(buf, buf_size, ANGBAND_DIR_FILE, _("dead_j.txt", "dead.txt"));

	FILE *fp;
	fp = my_fopen(buf, "r");
	if (!fp) return;

	int i = 0;
	while (my_fgets(fp, buf, buf_size) == 0)
	{
		put_str(buf, i++, 0);
	}

	my_fclose(fp);
}
