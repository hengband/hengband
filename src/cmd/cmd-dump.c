/*!
 * @file cmd-dump.c
 * @brief プレイヤーのインターフェイスに関するコマンドの実装 / Interface commands
 * @date 2014/01/02
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 * @details
 * <pre>
 * A set of functions to maintain automatic dumps of various kinds.
 * The dump commands of original Angband simply add new lines to
 * existing files; these files will become bigger and bigger unless
 * an user deletes some or all of these files by hand at some
 * point.
 * These three functions automatically delete old dumped lines
 * before adding new ones.  Since there are various kinds of automatic
 * dumps in a single file, we add a header and a footer with a type
 * name for every automatic dump, and kill old lines only when the
 * lines have the correct type of header and footer.
 * We need to be quite paranoid about correctness; the user might
 * (mistakenly) edit the file by hand, and see all their work come
 * to nothing on the next auto dump otherwise.  The current code only
 * detects changes by noting inconsistencies between the actual number
 * of lines and the number written in the footer.  Note that this will
 * not catch single-line edits.
 * </pre>
 */

#include "angband.h"
#include "cmd/cmd-draw.h"
#include "cmd/cmd-dump.h"
#include "cmd/cmd-inventory.h"
#include "cmd/lighting-level-table.h"
#include "cmd/cmd-visuals.h"
#include "cmd/dump-util.h"
#include "gameterm.h"
#include "core.h" // 暫定。後で消す.
#include "core/show-file.h"
#include "io/read-pref-file.h"
#include "io/interpret-pref-file.h"

#include "knowledge/knowledge-items.h"
#include "knowledge/knowledge-experiences.h"
#include "knowledge/knowledge-features.h"
#include "knowledge/knowledge-monsters.h"
#include "knowledge/knowledge-quests.h"
#include "knowledge/knowledge-self.h"
#include "knowledge/knowledge-uniques.h"

#include "autopick.h"
#include "dungeon.h"
#include "world.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "player-personality.h"
#include "sort.h"
#include "mutation.h"
#include "quest.h"
#include "market/store.h"
#include "artifact.h"
#include "object-flavor.h"
#include "object/object-kind.h"
#include "floor-town.h"
#include "cmd/feeling-table.h"
#include "cmd/object-group-table.h"
#include "market/store-util.h"
#include "view-mainwindow.h" // 暫定。後で消す
#include "english.h"

#include "diary-subtitle-table.h"
#include "io/write-diary.h"
#include "chuukei.h"

/*!
 * @brief prefファイルを選択して処理する /
 * Ask for a "user pref line" and process it
 * @brief prf出力内容を消去する /
 * Remove old lines automatically generated before.
 * @param orig_file 消去を行うファイル名
 */
static void remove_auto_dump(concptr orig_file, concptr auto_dump_mark)
{
	char buf[1024];
	bool between_mark = FALSE;
	bool changed = FALSE;
	int line_num = 0;
	long header_location = 0;
	char header_mark_str[80];
	char footer_mark_str[80];

	sprintf(header_mark_str, auto_dump_header, auto_dump_mark);
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);
	size_t mark_len = strlen(footer_mark_str);

	FILE *orig_fff;
	orig_fff = my_fopen(orig_file, "r");
	if (!orig_fff) return;

	FILE *tmp_fff = NULL;
	char tmp_file[FILE_NAME_SIZE];
	if (!open_temporary_file(&tmp_fff, tmp_file)) return;

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


#ifdef JP
#else
/*!
 * @brief Return suffix of ordinal number
 * @param num number
 * @return pointer of suffix string.
 */
concptr get_ordinal_number_suffix(int num)
{
	num = ABS(num) % 100;
	switch (num % 10)
	{
	case 1:
		return (num == 11) ? "th" : "st";
	case 2:
		return (num == 12) ? "th" : "nd";
	case 3:
		return (num == 13) ? "th" : "rd";
	default:
		return "th";
	}
}
#endif

/*!
 * @brief 画面を再描画するコマンドのメインルーチン
 * Hack -- redraw the screen
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Allow absolute file names?
 */
void do_cmd_pref(player_type *creature_ptr)
{
	char buf[80];
	strcpy(buf, "");
	if (!get_string(_("設定変更コマンド: ", "Pref: "), buf, 80)) return;

	(void)interpret_pref_file(creature_ptr, buf);
}


/*!
 * @brief 自動拾い設定ファイルをロードするコマンドのメインルーチン /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_reload_autopick(player_type *creature_ptr)
{
	if (!get_check(_("自動拾い設定ファイルをロードしますか? ", "Reload auto-pick preference file? ")))
		return;

	autopick_load_pref(creature_ptr, TRUE);
}


/*
 * Interact with "colors"
 */
void do_cmd_colors(player_type *creature_ptr)
{
	int i;
	char tmp[160];
	char buf[1024];
	FILE *auto_dump_stream;
	FILE_TYPE(FILE_TYPE_TEXT);
	screen_save();
	while (TRUE)
	{
		Term_clear();
		prt(_("[ カラーの設定 ]", "Interact with Colors"), 2, 0);
		prt(_("(1) ユーザー設定ファイルのロード", "(1) Load a user pref file"), 4, 5);
		prt(_("(2) カラーの設定をファイルに書き出す", "(2) Dump colors"), 5, 5);
		prt(_("(3) カラーの設定を変更する", "(3) Modify colors"), 6, 5);
		prt(_("コマンド: ", "Command: "), 8, 0);
		i = inkey();
		if (i == ESCAPE) break;

		if (i == '1')
		{
			prt(_("コマンド: ユーザー設定ファイルをロードします", "Command: Load a user pref file"), 8, 0);
			prt(_("ファイル: ", "File: "), 10, 0);
			sprintf(tmp, "%s.prf", creature_ptr->base_name);
			if (!askfor(tmp, 70)) continue;

			(void)process_pref_file(creature_ptr, tmp);
			Term_xtra(TERM_XTRA_REACT, 0);
			Term_redraw();
		}
		else if (i == '2')
		{
			static concptr mark = "Colors";
			prt(_("コマンド: カラーの設定をファイルに書き出します", "Command: Dump colors"), 8, 0);
			prt(_("ファイル: ", "File: "), 10, 0);
			sprintf(tmp, "%s.prf", creature_ptr->base_name);
			if (!askfor(tmp, 70)) continue;

			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);
			if (!open_auto_dump(&auto_dump_stream, buf, mark)) continue;

			auto_dump_printf(auto_dump_stream, _("\n# カラーの設定\n\n", "\n# Color redefinitions\n\n"));
			for (i = 0; i < 256; i++)
			{
				int kv = angband_color_table[i][0];
				int rv = angband_color_table[i][1];
				int gv = angband_color_table[i][2];
				int bv = angband_color_table[i][3];

				concptr name = _("未知", "unknown");
				if (!kv && !rv && !gv && !bv) continue;

				if (i < 16) name = color_names[i];

				auto_dump_printf(auto_dump_stream, _("# カラー '%s'\n", "# Color '%s'\n"), name);
				auto_dump_printf(auto_dump_stream, "V:%d:0x%02X:0x%02X:0x%02X:0x%02X\n\n",
					i, kv, rv, gv, bv);
			}

			close_auto_dump(&auto_dump_stream, mark);
			msg_print(_("カラーの設定をファイルに書き出しました。", "Dumped color redefinitions."));
		}
		else if (i == '3')
		{
			static byte a = 0;
			prt(_("コマンド: カラーの設定を変更します", "Command: Modify colors"), 8, 0);
			while (TRUE)
			{
				concptr name;
				clear_from(10);
				for (byte j = 0; j < 16; j++)
				{
					Term_putstr(j * 4, 20, -1, a, "###");
					Term_putstr(j * 4, 22, -1, j, format("%3d", j));
				}

				name = ((a < 16) ? color_names[a] : _("未定義", "undefined"));
				Term_putstr(5, 10, -1, TERM_WHITE,
					format(_("カラー = %d, 名前 = %s", "Color = %d, Name = %s"), a, name));
				Term_putstr(5, 12, -1, TERM_WHITE,
					format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x",
						angband_color_table[a][0],
						angband_color_table[a][1],
						angband_color_table[a][2],
						angband_color_table[a][3]));
				Term_putstr(0, 14, -1, TERM_WHITE,
					_("コマンド (n/N/k/K/r/R/g/G/b/B): ", "Command (n/N/k/K/r/R/g/G/b/B): "));
				i = inkey();
				if (i == ESCAPE) break;

				if (i == 'n') a = (byte)(a + 1);
				if (i == 'N') a = (byte)(a - 1);
				if (i == 'k') angband_color_table[a][0] = (byte)(angband_color_table[a][0] + 1);
				if (i == 'K') angband_color_table[a][0] = (byte)(angband_color_table[a][0] - 1);
				if (i == 'r') angband_color_table[a][1] = (byte)(angband_color_table[a][1] + 1);
				if (i == 'R') angband_color_table[a][1] = (byte)(angband_color_table[a][1] - 1);
				if (i == 'g') angband_color_table[a][2] = (byte)(angband_color_table[a][2] + 1);
				if (i == 'G') angband_color_table[a][2] = (byte)(angband_color_table[a][2] - 1);
				if (i == 'b') angband_color_table[a][3] = (byte)(angband_color_table[a][3] + 1);
				if (i == 'B') angband_color_table[a][3] = (byte)(angband_color_table[a][3] - 1);

				Term_xtra(TERM_XTRA_REACT, 0);
				Term_redraw();
			}
		}
		else
		{
			bell();
		}

		msg_erase();
	}

	screen_load();
}


/*
 * Note something in the message recall
 */
void do_cmd_note(void)
{
	char buf[80];
	strcpy(buf, "");
	if (!get_string(_("メモ: ", "Note: "), buf, 60)) return;
	if (!buf[0] || (buf[0] == ' ')) return;

	msg_format(_("メモ: %s", "Note: %s"), buf);
}


/*
 * Mention the current version
 */
void do_cmd_version(void)
{
#if FAKE_VER_EXTRA > 0
	msg_format(_("変愚蛮怒(Hengband) %d.%d.%d.%d", "You are playing Hengband %d.%d.%d.%d."),
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH, FAKE_VER_EXTRA);
#else
	msg_format(_("変愚蛮怒(Hengband) %d.%d.%d", "You are playing Hengband %d.%d.%d."),
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif
}


/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(player_type *creature_ptr)
{
	if (creature_ptr->wild_mode) return;

	if (creature_ptr->current_floor_ptr->inside_quest && !random_quest_number(creature_ptr, creature_ptr->current_floor_ptr->dun_level))
	{
		msg_print(_("典型的なクエストのダンジョンのようだ。", "Looks like a typical quest level."));
		return;
	}

	if (creature_ptr->town_num && !creature_ptr->current_floor_ptr->dun_level)
	{
		if (!strcmp(town_info[creature_ptr->town_num].name, _("荒野", "wilderness")))
		{
			msg_print(_("何かありそうな荒野のようだ。", "Looks like a strange wilderness."));
			return;
		}

		msg_print(_("典型的な町のようだ。", "Looks like a typical town."));
		return;
	}

	if (!creature_ptr->current_floor_ptr->dun_level)
	{
		msg_print(_("典型的な荒野のようだ。", "Looks like a typical wilderness."));
		return;
	}

	if (creature_ptr->muta3 & MUT3_GOOD_LUCK)
		msg_print(do_cmd_feeling_text_lucky[creature_ptr->feeling]);
	else if (IS_ECHIZEN(creature_ptr))
		msg_print(do_cmd_feeling_text_combat[creature_ptr->feeling]);
	else
		msg_print(do_cmd_feeling_text[creature_ptr->feeling]);
}


/*
 * Check the status of "autopick"
 */
static void do_cmd_knowledge_autopick(player_type *creature_ptr)
{
	FILE *fff = NULL;
	GAME_TEXT file_name[FILE_NAME_SIZE];
	if (!open_temporary_file(&fff, file_name)) return;

	if (!max_autopick)
	{
		fprintf(fff, _("自動破壊/拾いには何も登録されていません。", "No preference for auto picker/destroyer."));
	}
	else
	{
		fprintf(fff, _("   自動拾い/破壊には現在 %d行登録されています。\n\n",
			"   There are %d registered lines for auto picker/destroyer.\n\n"), max_autopick);
	}

	for (int k = 0; k < max_autopick; k++)
	{
		concptr tmp;
		byte act = autopick_list[k].action;
		if (act & DONT_AUTOPICK)
		{
			tmp = _("放置", "Leave");
		}
		else if (act & DO_AUTODESTROY)
		{
			tmp = _("破壊", "Destroy");
		}
		else if (act & DO_AUTOPICK)
		{
			tmp = _("拾う", "Pickup");
		}
		else
		{
			tmp = _("確認", "Query");
		}

		if (act & DO_DISPLAY)
			fprintf(fff, "%11s", format("[%s]", tmp));
		else
			fprintf(fff, "%11s", format("(%s)", tmp));

		tmp = autopick_line_from_entry(&autopick_list[k]);
		fprintf(fff, " %s", tmp);
		string_free(tmp);
		fprintf(fff, "\n");
	}

	my_fclose(fff);

	(void)show_file(creature_ptr, TRUE, file_name, _("自動拾い/破壊 設定リスト", "Auto-picker/Destroyer"), 0, 0);
	fd_kill(file_name);
}


/*
 * Interact with "knowledge"
 */
void do_cmd_knowledge(player_type *creature_ptr)
{
	int i, p = 0;
	bool need_redraw = FALSE;
	FILE_TYPE(FILE_TYPE_TEXT);
	screen_save();
	while (TRUE)
	{
		Term_clear();
		prt(format(_("%d/2 ページ", "page %d/2"), (p + 1)), 2, 65);
		prt(_("現在の知識を確認する", "Display current knowledge"), 3, 0);
		if (p == 0)
		{
			prt(_("(1) 既知の伝説のアイテム                 の一覧", "(1) Display known artifacts"), 6, 5);
			prt(_("(2) 既知のアイテム                       の一覧", "(2) Display known objects"), 7, 5);
			prt(_("(3) 既知の生きているユニーク・モンスター の一覧", "(3) Display remaining uniques"), 8, 5);
			prt(_("(4) 既知のモンスター                     の一覧", "(4) Display known monster"), 9, 5);
			prt(_("(5) 倒した敵の数                         の一覧", "(5) Display kill count"), 10, 5);
			if (!vanilla_town) prt(_("(6) 賞金首                               の一覧", "(6) Display wanted monsters"), 11, 5);
			prt(_("(7) 現在のペット                         の一覧", "(7) Display current pets"), 12, 5);
			prt(_("(8) 我が家のアイテム                     の一覧", "(8) Display home inventory"), 13, 5);
			prt(_("(9) *鑑定*済み装備の耐性                 の一覧", "(9) Display *identified* equip."), 14, 5);
			prt(_("(0) 地形の表示文字/タイル                の一覧", "(0) Display terrain symbols."), 15, 5);
		}
		else
		{
			prt(_("(a) 自分に関する情報                     の一覧", "(a) Display about yourself"), 6, 5);
			prt(_("(b) 突然変異                             の一覧", "(b) Display mutations"), 7, 5);
			prt(_("(c) 武器の経験値                         の一覧", "(c) Display weapon proficiency"), 8, 5);
			prt(_("(d) 魔法の経験値                         の一覧", "(d) Display spell proficiency"), 9, 5);
			prt(_("(e) 技能の経験値                         の一覧", "(e) Display misc. proficiency"), 10, 5);
			prt(_("(f) プレイヤーの徳                       の一覧", "(f) Display virtues"), 11, 5);
			prt(_("(g) 入ったダンジョン                     の一覧", "(g) Display dungeons"), 12, 5);
			prt(_("(h) 実行中のクエスト                     の一覧", "(h) Display current quests"), 13, 5);
			prt(_("(i) 現在の自動拾い/破壊設定              の一覧", "(i) Display auto pick/destroy"), 14, 5);
		}

		prt(_("-続く-", "-more-"), 17, 8);
		prt(_("ESC) 抜ける", "ESC) Exit menu"), 21, 1);
		prt(_("SPACE) 次ページ", "SPACE) Next page"), 21, 30);
		prt(_("コマンド:", "Command: "), 20, 0);
		i = inkey();

		if (i == ESCAPE) break;
		switch (i)
		{
		case ' ': /* Page change */
		case '-':
			p = 1 - p;
			break;
		case '1': /* Artifacts */
			do_cmd_knowledge_artifacts(creature_ptr);
			break;
		case '2': /* Objects */
			do_cmd_knowledge_objects(creature_ptr, &need_redraw, FALSE, -1);
			break;
		case '3': /* Uniques */
			do_cmd_knowledge_uniques(creature_ptr);
			break;
		case '4': /* Monsters */
			do_cmd_knowledge_monsters(creature_ptr, &need_redraw, FALSE, -1);
			break;
		case '5': /* Kill count  */
			do_cmd_knowledge_kill_count(creature_ptr);
			break;
		case '6': /* wanted */
			if (!vanilla_town) do_cmd_knowledge_bounty(creature_ptr);
			break;
		case '7': /* Pets */
			do_cmd_knowledge_pets(creature_ptr);
			break;
		case '8': /* Home */
			do_cmd_knowledge_home(creature_ptr);
			break;
		case '9': /* Resist list */
			do_cmd_knowledge_inventory(creature_ptr);
			break;
		case '0': /* Feature list */
		{
			IDX lighting_level = F_LIT_STANDARD;
			do_cmd_knowledge_features(&need_redraw, FALSE, -1, &lighting_level);
		}
		break;
		/* Next page */
		case 'a': /* Max stat */
			do_cmd_knowledge_stat(creature_ptr);
			break;
		case 'b': /* Mutations */
			do_cmd_knowledge_mutations(creature_ptr);
			break;
		case 'c': /* weapon-exp */
			do_cmd_knowledge_weapon_exp(creature_ptr);
			break;
		case 'd': /* spell-exp */
			do_cmd_knowledge_spell_exp(creature_ptr);
			break;
		case 'e': /* skill-exp */
			do_cmd_knowledge_skill_exp(creature_ptr);
			break;
		case 'f': /* Virtues */
			do_cmd_knowledge_virtues(creature_ptr);
			break;
		case 'g': /* Dungeon */
			do_cmd_knowledge_dungeon(creature_ptr);
			break;
		case 'h': /* Quests */
			do_cmd_knowledge_quests(creature_ptr);
			break;
		case 'i': /* Autopick */
			do_cmd_knowledge_autopick(creature_ptr);
			break;
		default: /* Unknown option */
			bell();
		}

		msg_erase();
	}

	screen_load();
	if (need_redraw) do_cmd_redraw(creature_ptr);
}


/*
 * Check on the status of an active quest
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_checkquest(player_type *creature_ptr)
{
	FILE_TYPE(FILE_TYPE_TEXT);
	screen_save();
	do_cmd_knowledge_quests(creature_ptr);
	screen_load();
}


/*
 * Display the time and date
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_time(player_type *creature_ptr)
{
	int day, hour, min;
	extract_day_hour_min(creature_ptr, &day, &hour, &min);

	char desc[1024];
	strcpy(desc, _("変な時刻だ。", "It is a strange time."));

	char day_buf[10];
	if (day < MAX_DAYS) sprintf(day_buf, "%d", day);
	else strcpy(day_buf, "*****");

	msg_format(_("%s日目, 時刻は%d:%02d %sです。", "This is day %s. The time is %d:%02d %s."),
		day_buf, (hour % 12 == 0) ? 12 : (hour % 12), min, (hour < 12) ? "AM" : "PM");

	char buf[1024];
	if (!randint0(10) || creature_ptr->image)
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timefun_j.txt", "timefun.txt"));
	}
	else
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timenorm_j.txt", "timenorm.txt"));
	}

	FILE *fff;
	fff = my_fopen(buf, "rt");

	if (!fff) return;

	int full = hour * 100 + min;
	int start = 9999;
	int end = -9999;
	int num = 0;
	while (!my_fgets(fff, buf, sizeof(buf)))
	{
		if (!buf[0] || (buf[0] == '#')) continue;
		if (buf[1] != ':') continue;

		if (buf[0] == 'S')
		{
			start = atoi(buf + 2);
			end = start + 59;
			continue;
		}

		if (buf[0] == 'E')
		{
			end = atoi(buf + 2);
			continue;
		}

		if ((start > full) || (full > end)) continue;

		if (buf[0] == 'D')
		{
			num++;
			if (!randint0(num)) strcpy(desc, buf + 2);

			continue;
		}
	}

	msg_print(desc);
	my_fclose(fff);
}
