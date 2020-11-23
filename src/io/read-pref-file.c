/*
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

#include "io/read-pref-file.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "io-dump/dump-remover.h"
#include "io/files-util.h"
#include "io/interpret-pref-file.h"
#include "io/pref-file-expressor.h"
#include "player/player-class.h"
#include "player/player-race.h"
#include "realm/realm-names-table.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "util/buffer-shaper.h"
#include "view/display-messages.h"
#include "world/world.h"

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
static errr process_pref_file_aux(player_type *creature_ptr, concptr name, int preftype, void(*process_autopick_file_command)(char*))
{
	FILE *fp;
	fp = angband_fopen(name, "r");
	if (!fp) return -1;

	char buf[1024];
	char old[1024];
	int line = -1;
	errr err = 0;
	bool bypass = FALSE;
	while (angband_fgets(fp, buf, sizeof(buf)) == 0)
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
				(void)process_autopick_file(creature_ptr, buf + 2, process_autopick_file_command);
				break;
			case PREF_TYPE_HISTPREF:
				(void)process_histpref_file(creature_ptr, buf + 2, process_autopick_file_command);
				break;
			default:
				(void)process_pref_file(creature_ptr, buf + 2, process_autopick_file_command);
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
			
			(*process_autopick_file_command)(buf);
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

	angband_fclose(fp);
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
errr process_pref_file(player_type *creature_ptr, concptr name, void(*process_autopick_file_command)(char*))
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, name);

	errr err1 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL, process_autopick_file_command);
	if (err1 > 0) return err1;

	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	errr err2 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL, process_autopick_file_command);
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
errr process_autopick_file(player_type *creature_ptr, concptr name, void(*process_autopick_file_command)(char*))
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_AUTOPICK, process_autopick_file_command);
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
errr process_histpref_file(player_type *creature_ptr, concptr name, void(*process_autopick_file_command)(char*))
{
	bool old_character_xtra = current_world_ptr->character_xtra;
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* Hack -- prevent modification birth options in this file */
	current_world_ptr->character_xtra = TRUE;
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_HISTPREF, process_autopick_file_command);
	current_world_ptr->character_xtra = old_character_xtra;
	return err;
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
	*fpp = angband_fopen(buf, "a");
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
	angband_fclose(*fpp);
}

/*!
 * @brief 全ユーザプロファイルをロードする / Load some "user pref files"
 * @paaram player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note
 * Modified by Arcum Dagsson to support
 * separate macro files for different realms.
 */
void load_all_pref_files(player_type* player_ptr)
{
    char buf[1024];
    sprintf(buf, "user.prf");
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    sprintf(buf, "user-%s.prf", ANGBAND_SYS);
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    sprintf(buf, "%s.prf", rp_ptr->title);
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    sprintf(buf, "%s.prf", cp_ptr->title);
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    sprintf(buf, "%s.prf", player_ptr->base_name);
    process_pref_file(player_ptr, buf, process_autopick_file_command);
    if (player_ptr->realm1 != REALM_NONE) {
        sprintf(buf, "%s.prf", realm_names[player_ptr->realm1]);
        process_pref_file(player_ptr, buf, process_autopick_file_command);
    }

    if (player_ptr->realm2 != REALM_NONE) {
        sprintf(buf, "%s.prf", realm_names[player_ptr->realm2]);
        process_pref_file(player_ptr, buf, process_autopick_file_command);
    }

    autopick_load_pref(player_ptr, FALSE);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 * @return なし
 */
bool read_histpref(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    char buf[80];
    errr err;
    int i, j, n;
    char *s, *t;
    char temp[64 * 4];
    char histbuf[HISTPREF_LIMIT];

    if (!get_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? ")))
        return FALSE;

    histbuf[0] = '\0';
    histpref_buf = histbuf;

    sprintf(buf, _("histedit-%s.prf", "histpref-%s.prf"), creature_ptr->base_name);
    err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);

    if (0 > err) {
        strcpy(buf, _("histedit.prf", "histpref.prf"));
        err = process_histpref_file(creature_ptr, buf, process_autopick_file_command);
    }

    if (err) {
        msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
        msg_print(NULL);
        histpref_buf = NULL;
        return FALSE;
    } else if (!histpref_buf[0]) {
        msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
        msg_print(NULL);
        histpref_buf = NULL;
        return FALSE;
    }

    for (i = 0; i < 4; i++)
        creature_ptr->history[i][0] = '\0';

    /* loop */
    for (s = histpref_buf; *s == ' '; s++)
        ;

    n = strlen(s);
    while ((n > 0) && (s[n - 1] == ' '))
        s[--n] = '\0';

    shape_buffer(s, 60, temp, sizeof(temp));
    t = temp;
    for (i = 0; i < 4; i++) {
        if (t[0] == 0)
            break;
        else {
            strcpy(creature_ptr->history[i], t);
            t += strlen(t) + 1;
        }
    }

    for (i = 0; i < 4; i++) {
        /* loop */
        for (j = 0; creature_ptr->history[i][j]; j++)
            ;

        for (; j < 59; j++)
            creature_ptr->history[i][j] = ' ';
        creature_ptr->history[i][59] = '\0';
    }

    histpref_buf = NULL;
    return TRUE;
}
