/*!
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

#include "system/angband.h"
#include "io/uid-checker.h"
#include "io/files-util.h"
#include "system/system-variables.h" // 暫定。後で消す.
#include "io/character-dump.h"

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
 * @brief プレイヤーステータスをファイルダンプ出力する
 * Hack -- Dump a character description file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 出力ファイル名
 * @return エラーコード
 * @details
 * Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
errr file_character(player_type *creature_ptr, concptr name, update_playtime_pf update_playtime, display_player_pf display_player, map_name_pf map_name)
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
		else
		{
			msg_format("Error in line %d of %s!", line_num, file_name);
			my_fclose(fp);
			return -1;
		}
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
