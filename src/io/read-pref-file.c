#include "io/read-pref-file.h"
#include "io/process-pref-file.h"
#include "autopick.h"
#include "files.h" // 暫定。コールバック化して後で消す.
#include "world.h"

// todo コールバック関数に変更するので、いずれ消す.
#define PREF_TYPE_NORMAL   0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

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
