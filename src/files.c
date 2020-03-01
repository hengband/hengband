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
#include "io/gf-descriptions.h"
#include "io/tokenizer.h"

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
 * @brief 設定ファイルの各行から各種テキスト情報を取得する /
 * Parse a sub-file of the "extra info" (format shown below)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf データテキストの参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 * Parse another file recursively, see below for details
 *   %:\<filename\>
 * Specify the attr/char values for "monsters" by race index
 *   R:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "objects" by kind index
 *   K:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "features" by feature index
 *   F:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for unaware "objects" by kind tval
 *   U:\<tv\>:\<a\>:\<c\>
 * Specify the attr/char values for inventory "objects" by kind tval
 *   E:\<tv\>:\<a\>:\<c\>
 * Define a macro action, given an encoded macro action
 *   A:\<str\>
 * Create a normal macro, given an encoded macro trigger
 *   P:\<str\>
 * Create a command macro, given an encoded macro trigger
 *   C:\<str\>
 * Create a keyset mapping
 *   S:\<key\>:\<key\>:\<dir\>
 * Turn an option off, given its name
 *   X:\<str\>
 * Turn an option on, given its name
 *   Y:\<str\>
 * Specify visual information, given an index, and some data
 *   V:\<num\>:\<kv\>:\<rv\>:\<gv\>:\<bv\>
 * Specify the set of colors to use when drawing a zapped spell
 *   Z:\<type\>:\<str\>
 * Specify a macro trigger template and macro trigger names.
 *   T:\<template\>:\<modifier chr\>:\<modifier name1\>:\<modifier name2\>:...
 *   T:\<trigger\>:\<keycode\>:\<shift-keycode\>
 * </pre>
 */
errr process_pref_file_command(player_type *creature_ptr, char *buf)
{
	if (buf[1] != ':') return 1;

	char *zz[16];
	switch (buf[0])
	{
	case 'H':
	{
		/* Process "H:<history>" */
		add_history_from_pref_line(buf + 2);
		return 0;
	}
	case 'R':
	{
		/* Process "R:<num>:<a>/<c>" -- attr/char for monster races */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		monster_race *r_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_r_idx) return 1;
		r_ptr = &r_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) r_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) r_ptr->x_char = n2;
		return 0;
	}
	case 'K':
	{
		/* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;
		
		object_kind *k_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_k_idx) return 1;
		k_ptr = &k_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) k_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) k_ptr->x_char = n2;
		return 0;
	}
	case 'F':
	{
		/* Process "F:<num>:<a>/<c>" -- attr/char for terrain features */
		/* "F:<num>:<a>/<c>" */
		/* "F:<num>:<a>/<c>:LIT" */
		/* "F:<num>:<a>/<c>:<la>/<lc>:<da>/<dc>" */
		feature_type *f_ptr;
		int num = tokenize(buf + 2, F_LIT_MAX * 2 + 1, zz, TOKENIZE_CHECKQUOTE);

		if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) return 1;
		else if ((num == 4) && !streq(zz[3], "LIT")) return 1;

		int i = (huge)strtol(zz[0], NULL, 0);
		if (i >= max_f_idx) return 1;
		f_ptr = &f_info[i];

		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[F_LIT_STANDARD] = n1; /* Allow TERM_DARK text */
		if (n2) f_ptr->x_char[F_LIT_STANDARD] = n2;

		switch (num)
		{
		case 3:
		{
			/* No lighting support */
			n1 = f_ptr->x_attr[F_LIT_STANDARD];
			n2 = f_ptr->x_char[F_LIT_STANDARD];
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				f_ptr->x_attr[j] = n1;
				f_ptr->x_char[j] = n2;
			}

			return 0;
		}
		case 4:
		{
			/* Use default lighting */
			apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
			return 0;
		}
		case F_LIT_MAX * 2 + 1:
		{
			/* Use desired lighting */
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				n1 = (TERM_COLOR)strtol(zz[j * 2 + 1], NULL, 0);
				n2 = (SYMBOL_CODE)strtol(zz[j * 2 + 2], NULL, 0);
				if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[j] = n1; /* Allow TERM_DARK text */
				if (n2) f_ptr->x_char[j] = n2;
			}

			return 0;
		}
		default:
			return 0;
		}
	}
	case 'S':
	{
		/* Process "S:<num>:<a>/<c>" -- attr/char for special things */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		int j = (byte)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		misc_to_attr[j] = n1;
		misc_to_char[j] = n2;
		return 0;
	}
	case 'U':
	{
		/* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) return 1;

		int j = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		for (int i = 1; i < max_k_idx; i++)
		{
			object_kind *k_ptr = &k_info[i];
			if (k_ptr->tval == j)
			{
				if (n1) k_ptr->d_attr = n1;
				if (n2) k_ptr->d_char = n2;
			}
		}

		return 0;
	}
	case 'E':
	{
		/* Process "E:<tv>:<a>" -- attribute for inventory objects */
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		int j = (byte)strtol(zz[0], NULL, 0) % 128;
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		if (n1) tval_to_attr[j] = n1;
		return 0;
	}
	case 'A':
	{
		/* Process "A:<str>" -- save an "action" for later */
		text_to_ascii(macro__buf, buf + 2);
		return 0;
	}
	case 'P':
	{
		/* Process "P:<str>" -- normal macro */
		char tmp[1024];
		text_to_ascii(tmp, buf + 2);
		macro_add(tmp, macro__buf);
		return 0;
	}
	case 'C':
	{
		/* Process "C:<str>" -- create keymap */
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		int mode = strtol(zz[0], NULL, 0);
		if ((mode < 0) || (mode >= KEYMAP_MODES)) return 1;

		char tmp[1024];
		text_to_ascii(tmp, zz[1]);
		if (!tmp[0] || tmp[1]) return 1;

		int i = (byte)(tmp[0]);
		string_free(keymap_act[mode][i]);
		keymap_act[mode][i] = string_make(macro__buf);
		return 0;
	}
	case 'V':
	{
		/* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info */
		if (tokenize(buf + 2, 5, zz, TOKENIZE_CHECKQUOTE) != 5) return 1;

		int i = (byte)strtol(zz[0], NULL, 0);
		angband_color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
		angband_color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
		angband_color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
		angband_color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
		return 0;
	}
	case 'X':
	case 'Y':
	{
		/* Process "X:<str>" -- turn option off */
		/* Process "Y:<str>" -- turn option on */
		for (int i = 0; option_info[i].o_desc; i++)
		{
			bool is_option = option_info[i].o_var != NULL;
			is_option &= option_info[i].o_text != NULL;
			is_option &= streq(option_info[i].o_text, buf + 2);
			if (!is_option) continue;

			int os = option_info[i].o_set;
			int ob = option_info[i].o_bit;

			if ((creature_ptr->playing || current_world_ptr->character_xtra) &&
				(OPT_PAGE_BIRTH == option_info[i].o_page) && !current_world_ptr->wizard)
			{
				msg_format(_("初期オプションは変更できません! '%s'", "Birth options can not changed! '%s'"), buf);
				msg_print(NULL);
				return 0;
			}

			if (buf[0] == 'X')
			{
				option_flag[os] &= ~(1L << ob);
				(*option_info[i].o_var) = FALSE;
				return 0;
			}

			option_flag[os] |= (1L << ob);
			(*option_info[i].o_var) = TRUE;
			return 0;
		}

		msg_format(_("オプションの名前が正しくありません： %s", "Ignored invalid option: %s"), buf);
		msg_print(NULL);
		return 0;
	}
	case 'Z':
	{
		/* Process "Z:<type>:<str>" -- set spell color */
		char *t = my_strchr(buf + 2, ':');
		if (!t) return 1;

		*(t++) = '\0';
		for (int i = 0; i < MAX_NAMED_NUM; i++)
		{
			if (!streq(gf_desc[i].name, buf + 2)) continue;

			gf_color[gf_desc[i].num] = (TERM_COLOR)quark_add(t);
			return 0;
		}

		return 1;
	}
	case 'T':
	{
		/* Initialize macro trigger names and a template */
		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		int tok = tokenize(buf + 2, 2 + MAX_MACRO_MOD, zz, 0);

		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		if (tok >= 4)
		{
			if (macro_template != NULL)
			{
				int macro_modifier_length = strlen(macro_modifier_chr);
				string_free(macro_template);
				macro_template = NULL;
				string_free(macro_modifier_chr);
				for (int i = 0; i < macro_modifier_length; i++)
				{
					string_free(macro_modifier_name[i]);
				}

				for (int i = 0; i < max_macrotrigger; i++)
				{
					string_free(macro_trigger_name[i]);
					string_free(macro_trigger_keycode[0][i]);
					string_free(macro_trigger_keycode[1][i]);
				}

				max_macrotrigger = 0;
			}

			if (*zz[0] == '\0') return 0;

			int zz_length = strlen(zz[1]);
			zz_length = MIN(MAX_MACRO_MOD, zz_length);
			if (2 + zz_length != tok) return 1;

			macro_template = string_make(zz[0]);
			macro_modifier_chr = string_make(zz[1]);
			for (int i = 0; i < zz_length; i++)
			{
				macro_modifier_name[i] = string_make(zz[2 + i]);
			}

			return 0;
		}

		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		if (tok < 2) return 0;

		char buf_aux[1024];
		char *t, *s;
		if (max_macrotrigger >= MAX_MACRO_TRIG)
		{
			msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
			return 1;
		}

		int m = max_macrotrigger;
		max_macrotrigger++;
		t = buf_aux;
		s = zz[0];
		while (*s)
		{
			if ('\\' == *s) s++;
			*t++ = *s++;
		}

		*t = '\0';
		macro_trigger_name[m] = string_make(buf_aux);
		macro_trigger_keycode[0][m] = string_make(zz[1]);
		if (tok == 3)
		{
			macro_trigger_keycode[1][m] = string_make(zz[2]);
			return 0;
		}

		macro_trigger_keycode[1][m] = string_make(zz[1]);
		return 0;
	}
	}

	return 1;
}


/*!
 * @brief process_pref_fileのサブルーチンとして条件分岐処理の解釈と結果を返す /
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
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
void do_cmd_help(player_type *creature_ptr)
{
	screen_save();
	(void)show_file(creature_ptr, TRUE, _("jhelp.hlp", "help.hlp"), NULL, 0, 0);
	screen_load();
}


/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @return なし
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(player_type *creature_ptr, bool sf)
{
	char old_player_base[32] = "";
	if (current_world_ptr->character_generated)
		strcpy(old_player_base, creature_ptr->base_name);

	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		if (iskanji(creature_ptr->name[i]))
		{
			i++;
			continue;
		}

		if (iscntrl((unsigned char)creature_ptr->name[i]))
#else
		if (iscntrl(creature_ptr->name[i]))
#endif
		{
			quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), creature_ptr->name);
		}
	}

	int k = 0;
	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		unsigned char c = creature_ptr->name[i];
#else
		char c = creature_ptr->name[i];
#endif

#ifdef JP
		if (iskanji(c)) {
			if (k + 2 >= sizeof(creature_ptr->base_name) || !creature_ptr->name[i + 1])
				break;

			creature_ptr->base_name[k++] = c;
			i++;
			creature_ptr->base_name[k++] = creature_ptr->name[i];
		}
#ifdef SJIS
		else if (iskana(c)) creature_ptr->base_name[k++] = c;
#endif
		else
#endif
			if (!strncmp(PATH_SEP, creature_ptr->name + i, strlen(PATH_SEP)))
			{
				creature_ptr->base_name[k++] = '_';
				i += strlen(PATH_SEP);
			}
#if defined(WINDOWS)
			else if (my_strchr("\"*,/:;<>?\\|", c))
				creature_ptr->base_name[k++] = '_';
#endif
			else if (isprint(c))
				creature_ptr->base_name[k++] = c;
	}

	creature_ptr->base_name[k] = '\0';
	if (!creature_ptr->base_name[0])
		strcpy(creature_ptr->base_name, "PLAYER");

#ifdef SAVEFILE_MUTABLE
	sf = TRUE;
#endif
	if (!savefile_base[0] && savefile[0])
	{
		concptr s = savefile;
		while (TRUE)
		{
			concptr t;
			t = my_strstr(s, PATH_SEP);
			if (!t)
				break;
			s = t + 1;
		}

		strcpy(savefile_base, s);
	}

	if (!savefile_base[0] || !savefile[0])
		sf = TRUE;

	if (sf)
	{
		char temp[128];
		strcpy(savefile_base, creature_ptr->base_name);

#ifdef SAVEFILE_USE_UID
		/* Rename the savefile, using the creature_ptr->player_uid and creature_ptr->base_name */
		(void)sprintf(temp, "%d.%s", creature_ptr->player_uid, creature_ptr->base_name);
#else
		/* Rename the savefile, using the creature_ptr->base_name */
		(void)sprintf(temp, "%s", creature_ptr->base_name);
#endif
		path_build(savefile, sizeof(savefile), ANGBAND_DIR_SAVE, temp);
	}

	if (current_world_ptr->character_generated && !streq(old_player_base, creature_ptr->base_name))
	{
		autopick_load_pref(creature_ptr, FALSE);
	}
}


/*!
 * @brief プレイヤーの名前を変更するコマンドのメインルーチン
 * Gets a name for the character, reacting to name changes.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Assumes that "display_player()" has just been called
 * Perhaps we should NOT ask for a name (at "birth()") on
 * Unix machines?  XXX XXX
 * What a horrible name for a global function.
 * </pre>
 */
void get_name(player_type *creature_ptr)
{
	char tmp[64];
	strcpy(tmp, creature_ptr->name);

	if (get_string(_("キャラクターの名前を入力して下さい: ", "Enter a name for your character: "), tmp, 15))
	{
		strcpy(creature_ptr->name, tmp);
	}

	if (strlen(creature_ptr->name) == 0)
	{
		strcpy(creature_ptr->name, "PLAYER");
	}

	strcpy(tmp, ap_ptr->title);
#ifdef JP
	if (ap_ptr->no == 1)
		strcat(tmp, "の");
#else
	strcat(tmp, " ");
#endif
	strcat(tmp, creature_ptr->name);

	Term_erase(34, 1, 255);
	c_put_str(TERM_L_BLUE, tmp, 1, 34);
	clear_from(22);
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
