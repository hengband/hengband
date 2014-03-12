/*!
 * @file cmd4.c
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
 * -Mogami-
 * remove_auto_dump(orig_file, mark)
 *     Remove the old automatic dump of type "mark".
 * auto_dump_printf(fmt, ...)
 *     Dump a formatted string using fprintf().
 * open_auto_dump(buf, mark)
 *     Open a file, remove old dump, and add new header.
 * close_auto_dump(void)
 *     Add a footer, and close the file.
 *    The dump commands of original Angband simply add new lines to
 * existing files; these files will become bigger and bigger unless
 * an user deletes some or all of these files by hand at some
 * point.
 *     These three functions automatically delete old dumped lines 
 * before adding new ones.  Since there are various kinds of automatic 
 * dumps in a single file, we add a header and a footer with a type 
 * name for every automatic dump, and kill old lines only when the 
 * lines have the correct type of header and footer.
 *     We need to be quite paranoid about correctness; the user might 
 * (mistakenly) edit the file by hand, and see all their work come
 * to nothing on the next auto dump otherwise.  The current code only 
 * detects changes by noting inconsistencies between the actual number 
 * of lines and the number written in the footer.  Note that this will 
 * not catch single-line edits.
 * </pre>
 */

#include "angband.h"



/*
 */

/*
 *  Mark strings for auto dump
 */
static char auto_dump_header[] = "# vvvvvvv== %s ==vvvvvvv";
static char auto_dump_footer[] = "# ^^^^^^^== %s ==^^^^^^^";

/*
 * Variables for auto dump
 */
static FILE *auto_dump_stream;
static cptr auto_dump_mark;
static int auto_dump_line_num;


/*!
 * @brief prf出力内容を消去する /
 * Remove old lines automatically generated before.
 * @param orig_file 消去を行うファイル名
 */
static void remove_auto_dump(cptr orig_file)
{
	FILE *tmp_fff, *orig_fff;

	char tmp_file[1024];
	char buf[1024];
	bool between_mark = FALSE;
	bool changed = FALSE;
	int line_num = 0;
	long header_location = 0;
	char header_mark_str[80];
	char footer_mark_str[80];
	size_t mark_len;

	/* Prepare a header/footer mark string */
	sprintf(header_mark_str, auto_dump_header, auto_dump_mark);
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);

	mark_len = strlen(footer_mark_str);

	/* Open an old dump file in read-only mode */
	orig_fff = my_fopen(orig_file, "r");

	/* If original file does not exist, nothing to do */
	if (!orig_fff) return;

	/* Open a new (temporary) file */
	tmp_fff = my_fopen_temp(tmp_file, 1024);

	if (!tmp_fff)
	{
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), tmp_file);
	    msg_print(NULL);
	    return;
	}

	/* Loop for every line */
	while (TRUE)
	{
		/* Read a line */
		if (my_fgets(orig_fff, buf, sizeof(buf)))
		{
			/* Read error: Assume End of File */

			/*
			 * Was looking for the footer, but not found.
			 *
			 * Since automatic dump might be edited by hand,
			 * it's dangerous to kill these lines.
			 * Seek back to the next line of the (pseudo) header,
			 * and read again.
			 */
			if (between_mark)
			{
				fseek(orig_fff, header_location, SEEK_SET);
				between_mark = FALSE;
				continue;
			}

			/* Success -- End the loop */
			else
			{
				break;
			}
		}

		/* We are looking for the header mark of automatic dump */
		if (!between_mark)
		{
			/* Is this line a header? */
			if (!strcmp(buf, header_mark_str))
			{
				/* Memorise seek point of this line */
				header_location = ftell(orig_fff);

				/* Initialize counter for number of lines */
				line_num = 0;

				/* Look for the footer from now */
				between_mark = TRUE;

				/* There are some changes */
				changed = TRUE;
			}

			/* Not a header */
			else
			{
				/* Copy orginally lines */
				fprintf(tmp_fff, "%s\n", buf);
			}
		}

		/* We are looking for the footer mark of automatic dump */
		else
		{
			/* Is this line a footer? */
			if (!strncmp(buf, footer_mark_str, mark_len))
			{
				int tmp;

				/*
				 * Compare the number of lines
				 *
				 * If there is an inconsistency between
				 * actual number of lines and the
				 * number here, the automatic dump
				 * might be edited by hand.  So it's
				 * dangerous to kill these lines.
				 * Seek back to the next line of the
				 * (pseudo) header, and read again.
				 */
				if (!sscanf(buf + mark_len, " (%d)", &tmp)
				    || tmp != line_num)
				{
					fseek(orig_fff, header_location, SEEK_SET);
				}

				/* Look for another header */
				between_mark = FALSE;
			}

			/* Not a footer */
			else
			{
				/* Ignore old line, and count number of lines */
				line_num++;
			}
		}
	}

	/* Close files */
	my_fclose(orig_fff);
	my_fclose(tmp_fff);

	/* If there are some changes, overwrite the original file with new one */
	if (changed)
	{
		/* Copy contents of temporary file */

		tmp_fff = my_fopen(tmp_file, "r");
		orig_fff = my_fopen(orig_file, "w");

		while (!my_fgets(tmp_fff, buf, sizeof(buf)))
			fprintf(orig_fff, "%s\n", buf);

		my_fclose(orig_fff);
		my_fclose(tmp_fff);
	}

	/* Kill the temporary file */
	fd_kill(tmp_file);

	return;
}


/*!
 * @brief prfファイルのフォーマットに従った内容を出力する /
 * Dump a formatted line, using "vstrnfmt()".
 * @param fmt 出力内容
 */
static void auto_dump_printf(cptr fmt, ...)
{
	cptr p;
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Count number of lines */
	for (p = buf; *p; p++)
	{
		if (*p == '\n') auto_dump_line_num++;
	}

	/* Dump it */
	fprintf(auto_dump_stream, "%s", buf);
}


/*!
 * @brief prfファイルをファイルオープンする /
 * Open file to append auto dump.
 * @param buf ファイル名
 * @param mark 出力するヘッダマーク
 * @return ファイルポインタを取得できたらTRUEを返す
 */
static bool open_auto_dump(cptr buf, cptr mark)
{

	char header_mark_str[80];

	/* Save the mark string */
	auto_dump_mark = mark;

	/* Prepare a header mark string */
	sprintf(header_mark_str, auto_dump_header, auto_dump_mark);

	/* Remove old macro dumps */
	remove_auto_dump(buf);

	/* Append to the file */
	auto_dump_stream = my_fopen(buf, "a");

	/* Failure */
	if (!auto_dump_stream) {
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), buf);
		msg_print(NULL);

		/* Failed */
		return FALSE;
	}

	/* Start dumping */
	fprintf(auto_dump_stream, "%s\n", header_mark_str);

	/* Initialize counter */
	auto_dump_line_num = 0;

	auto_dump_printf(_("# *警告!!* 以降の行は自動生成されたものです。\n",
					   "# *Warning!*  The lines below are an automatic dump.\n"));
	auto_dump_printf(_("# *警告!!* 後で自動的に削除されるので編集しないでください。\n", 
					   "# Don't edit them; changes will be deleted and replaced automatically.\n"));
	/* Success */
	return TRUE;
}

/*!
 * @brief prfファイルをファイルクローズする /
 * Append foot part and close auto dump.
 * @return なし
 */
static void close_auto_dump(void)
{
	char footer_mark_str[80];

	/* Prepare a footer mark string */
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);

	auto_dump_printf(_("# *警告!!* 以降の行は自動生成されたものです。\n",
					   "# *Warning!*  The lines below are an automatic dump.\n"));
	auto_dump_printf(_("# *警告!!* 後で自動的に削除されるので編集しないでください。\n", 
					   "# Don't edit them; changes will be deleted and replaced automatically.\n"));
	/* End of dump */
	fprintf(auto_dump_stream, "%s (%d)\n", footer_mark_str, auto_dump_line_num);

	/* Close */
	my_fclose(auto_dump_stream);

	return;
}


#ifndef JP

/*!
 * @brief Return suffix of ordinal number
 * @param num number
 * @return pointer of suffix string.
 */
cptr get_ordinal_number_suffix(int num)
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
 * @brief 日記にメッセージを追加する /
 * Take note to the diary.
 * @param type 日記内容のID
 * @param num 日記内容のIDに応じた数値
 * @param note 日記内容のIDに応じた文字列参照ポインタ
 * @return エラーID
 */
errr do_cmd_write_nikki(int type, int num, cptr note)
{
	int day, hour, min;
	FILE *fff = NULL;
	char file_name[80];
	char buf[1024];
	cptr note_level = "";
	bool do_level = TRUE;
	char note_level_buf[40];
	int q_idx;

	static bool disable_nikki = FALSE;

	extract_day_hour_min(&day, &hour, &min);

	if (disable_nikki) return(-1);

	if (type == NIKKI_FIX_QUEST_C ||
	    type == NIKKI_FIX_QUEST_F ||
	    type == NIKKI_RAND_QUEST_C ||
	    type == NIKKI_RAND_QUEST_F ||
	    type == NIKKI_TO_QUEST)
	{
		int old_quest;

		old_quest = p_ptr->inside_quest;
		p_ptr->inside_quest = (quest[num].type == QUEST_TYPE_RANDOM) ? 0 : num;

		/* Get the quest text */
		init_flags = INIT_NAME_ONLY;

		process_dungeon_file("q_info.txt", 0, 0, 0, 0);

		/* Reset the old quest number */
		p_ptr->inside_quest = old_quest;
	}

	/* different filne name to avoid mixing */
	sprintf(file_name,_("playrecord-%s.txt", "playrec-%s.txt"),savefile_base);

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff)
	{
		msg_format(_("%s を開くことができませんでした。プレイ記録を一時停止します。", "Failed to open %s. Play-Record is disabled temporally."), buf);
		msg_format(NULL);
		disable_nikki=TRUE;
		return (-1);
	}

	q_idx = quest_number(dun_level);

	if (write_level)
	{
		if (p_ptr->inside_arena)
			note_level = _("アリーナ:", "Arane:");
		else if (!dun_level)
			note_level = _("地上:", "Surface:");
		else if (q_idx && (is_fixed_quest_idx(q_idx)
		         && !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
			note_level = _("クエスト:", "Quest:");
		else
		{
#ifdef JP
			sprintf(note_level_buf, "%d階(%s):", dun_level, d_name+d_info[dungeon_type].name);
#else
			sprintf(note_level_buf, "%s L%d:", d_name+d_info[dungeon_type].name, dun_level);
#endif
			note_level = note_level_buf;
		}
	}

	switch(type)
	{
		case NIKKI_HIGAWARI:
		{
			if (day < MAX_DAYS) fprintf(fff, _("%d日目\n", "Day %d\n"), day);
			else fputs(_("*****日目\n", "Day *****\n"), fff);
			do_level = FALSE;
			break;
		}
		case NIKKI_BUNSHOU:
		{
			if (num)
			{
				fprintf(fff, "%s\n",note);
				do_level = FALSE;
			}
			else
				fprintf(fff, " %2d:%02d %20s %s\n",hour, min, note_level, note);
			break;
		}
		case NIKKI_ART:
		{
			fprintf(fff, _(" %2d:%02d %20s %sを発見した。\n", " %2d:%02d %20s discovered %s.\n"), hour, min, note_level, note);
			break;
		}
		case NIKKI_UNIQUE:
		{
			fprintf(fff, _(" %2d:%02d %20s %sを倒した。\n", " %2d:%02d %20s defeated %s.\n"), hour, min, note_level, note);
			break;
		}
		case NIKKI_FIX_QUEST_C:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
			fprintf(fff, _(" %2d:%02d %20s クエスト「%s」を達成した。\n",
						   " %2d:%02d %20s completed quest '%s'.\n"), hour, min, note_level, quest[num].name);
			break;
		}
		case NIKKI_FIX_QUEST_F:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
			fprintf(fff, _(" %2d:%02d %20s クエスト「%s」から命からがら逃げ帰った。\n",
						   " %2d:%02d %20s run away from quest '%s'.\n"), hour, min, note_level, quest[num].name);
			break;
		}
		case NIKKI_RAND_QUEST_C:
		{
			char name[80];
			strcpy(name, r_name+r_info[quest[num].r_idx].name);
			fprintf(fff, _(" %2d:%02d %20s ランダムクエスト(%s)を達成した。\n",
						   " %2d:%02d %20s completed random quest '%s'\n"), hour, min, note_level, name);
			break;
		}
		case NIKKI_RAND_QUEST_F:
		{
			char name[80];
			strcpy(name, r_name+r_info[quest[num].r_idx].name);
			fprintf(fff, _(" %2d:%02d %20s ランダムクエスト(%s)から逃げ出した。\n",
						   " %2d:%02d %20s ran away from quest '%s'.\n"), hour, min, note_level, name);
			break;
		}
		case NIKKI_MAXDEAPTH:
		{
			fprintf(fff, _(" %2d:%02d %20s %sの最深階%d階に到達した。\n",
						   " %2d:%02d %20s reached level %d of %s for the first time.\n"), hour, min, note_level, d_name+d_info[dungeon_type].name, num);
			break;
		}
		case NIKKI_TRUMP:
		{
			fprintf(fff, _(" %2d:%02d %20s %s%sの最深階を%d階にセットした。\n",
						   " %2d:%02d %20s reset recall level of %s to %d %s.\n"), hour, min, note_level, note, d_name + d_info[num].name, max_dlv[num]);
			break;
		}
		case NIKKI_STAIR:
		{
			cptr to;
			if (q_idx && (is_fixed_quest_idx(q_idx)
			     && !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
			{
				to = _("地上", "the surface");
			}
			else
			{
				if (!(dun_level+num)) to = _("地上", "the surface");
				else to = format(_("%d階", "level %d"), dun_level+num);
			}
			fprintf(fff, _(" %2d:%02d %20s %sへ%s。\n", " %2d:%02d %20s %s %s.\n"), hour, min, note_level, _(to, note), _(note, to));
			break;
		}
		case NIKKI_RECALL:
		{
			if (!num)
			fprintf(fff, _(" %2d:%02d %20s 帰還を使って%sの%d階へ下りた。\n", " %2d:%02d %20s recalled to dungeon level %d of %s.\n"), 
						hour, min, note_level, _(d_name+d_info[dungeon_type].name, max_dlv[dungeon_type]), 
											   _(max_dlv[dungeon_type], d_name+d_info[dungeon_type].name));
			else
				fprintf(fff, _(" %2d:%02d %20s 帰還を使って地上へと戻った。\n", " %2d:%02d %20s recalled from dungeon to surface.\n"), hour, min, note_level);
			break;
		}
		case NIKKI_TO_QUEST:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
			fprintf(fff, _(" %2d:%02d %20s クエスト「%s」へと突入した。\n", " %2d:%02d %20s entered the quest '%s'.\n"),
						hour, min, note_level, quest[num].name);
			break;
		}
		case NIKKI_TELE_LEV:
		{
			fprintf(fff, _(" %2d:%02d %20s レベル・テレポートで脱出した。\n", " %2d:%02d %20s Got out using teleport level.\n"),
						hour, min, note_level);
			break;
		}
		case NIKKI_BUY:
		{
			fprintf(fff, _(" %2d:%02d %20s %sを購入した。\n", " %2d:%02d %20s bought %s.\n"), hour, min, note_level, note);
			break;
		}
		case NIKKI_SELL:
		{
			fprintf(fff, _(" %2d:%02d %20s %sを売却した。\n", " %2d:%02d %20s sold %s.\n"), hour, min, note_level, note);
			break;
		}
		case NIKKI_ARENA:
		{
			if (num < 0)
			{
				int n = -num;
				fprintf(fff, _(" %2d:%02d %20s 闘技場の%d回戦で、%sの前に敗れ去った。\n", " %2d:%02d %20s beaten by %s in the %d%s fight.\n"),
							hour, min, note_level, _(n, note), _(note, n), _("", get_ordinal_number_suffix(n)));
				break;
			}
			fprintf(fff, _(" %2d:%02d %20s 闘技場の%d回戦(%s)に勝利した。\n", " %2d:%02d %20s won the %d%s fight (%s).\n"), 
						hour, min, note_level, num, _(note, get_ordinal_number_suffix(num)), note);
			
			if (num == MAX_ARENA_MONS)
			{
				fprintf(fff, _("                 闘技場のすべての敵に勝利し、チャンピオンとなった。\n",
							   "                 won all fight to become a Chanpion.\n"));
				do_level = FALSE;
			}
			break;
		}
		case NIKKI_HANMEI:
		{
			fprintf(fff, _(" %2d:%02d %20s %sを識別した。\n", " %2d:%02d %20s identified %s.\n"), hour, min, note_level, note);
			break;
		}
		case NIKKI_WIZ_TELE:
		{
			cptr to;
			if (!dun_level)
				to = _("地上", "the surface");
			else
				to = format(_("%d階(%s)", "level %d of %s"), dun_level, d_name+d_info[dungeon_type].name);

			fprintf(fff, _(" %2d:%02d %20s %sへとウィザード・テレポートで移動した。\n",
						   " %2d:%02d %20s wizard-teleport to %s.\n"), hour, min, note_level, to);
			break;
		}
		case NIKKI_PAT_TELE:
		{
			cptr to;
			if (!dun_level)
				to = _("地上", "the surface");
			else
				to = format(_("%d階(%s)", "level %d of %s"), dun_level, d_name+d_info[dungeon_type].name);

			fprintf(fff, _(" %2d:%02d %20s %sへとパターンの力で移動した。\n",
						   " %2d:%02d %20s used Pattern to teleport to %s.\n"), hour, min, note_level, to);
			break;
		}
		case NIKKI_LEVELUP:
		{
			fprintf(fff, _(" %2d:%02d %20s レベルが%dに上がった。\n", " %2d:%02d %20s reached player level %d.\n"), hour, min, note_level, num);
			break;
		}
		case NIKKI_GAMESTART:
		{
			time_t ct = time((time_t*)0);
			do_level = FALSE;
			if (num)
			{
				fprintf(fff, "%s %s",note, ctime(&ct));
			}
			else
				fprintf(fff, " %2d:%02d %20s %s %s",hour, min, note_level, note, ctime(&ct));
			break;
		}
		case NIKKI_NAMED_PET:
		{
			fprintf(fff, " %2d:%02d %20s ", hour, min, note_level);
			switch (num)
			{
				case RECORD_NAMED_PET_NAME:
					fprintf(fff, _("%sを旅の友にすることに決めた。\n", "decided to travel together with %s.\n"), note);
					break;
				case RECORD_NAMED_PET_UNNAME:
					fprintf(fff, _("%sの名前を消した。\n", "unnamed %s.\n"), note);
					break;
				case RECORD_NAMED_PET_DISMISS:
					fprintf(fff, _("%sを解放した。\n", "dismissed %s.\n"), note);
					break;
				case RECORD_NAMED_PET_DEATH:
					fprintf(fff, _("%sが死んでしまった。\n", "%s died.\n"), note);
					break;
				case RECORD_NAMED_PET_MOVED:
					fprintf(fff, _("%sをおいて別のマップへ移動した。\n", "moved to another map leaving %s behind.\n"), note);
					break;
				case RECORD_NAMED_PET_LOST_SIGHT:
					fprintf(fff, _("%sとはぐれてしまった。\n", "lost sight of %s.\n"), note);
					break;
				case RECORD_NAMED_PET_DESTROY:
					fprintf(fff, _("%sが*破壊*によって消え去った。\n", "%s was made disappeared by *destruction*.\n"), note);
					break;
				case RECORD_NAMED_PET_EARTHQUAKE:
					fprintf(fff, _("%sが岩石に押し潰された。\n", "%s was crushed by falling rocks.\n"), note);
					break;
				case RECORD_NAMED_PET_GENOCIDE:
					fprintf(fff, _("%sが抹殺によって消え去った。\n", "%s was made disappeared by genocide.\n"), note);
					break;
				case RECORD_NAMED_PET_WIZ_ZAP:
					fprintf(fff, _("%sがデバッグコマンドによって消え去った。\n", "%s was removed by debug command.\n"), note);
					break;
				case RECORD_NAMED_PET_TELE_LEVEL:
					fprintf(fff, _("%sがテレポート・レベルによって消え去った。\n", "%s was made disappeared by teleport level.\n"), note);
					break;
				case RECORD_NAMED_PET_BLAST:
					fprintf(fff, _("%sを爆破した。\n", "blasted %s.\n"), note);
					break;
				case RECORD_NAMED_PET_HEAL_LEPER:
					fprintf(fff, _("%sの病気が治り旅から外れた。\n", "%s was healed and left.\n"), note);
					break;
				case RECORD_NAMED_PET_COMPACT:
					fprintf(fff, _("%sがモンスター情報圧縮によって消え去った。\n", "%s was made disappeared by compacting monsters.\n"), note);
					break;
				case RECORD_NAMED_PET_LOSE_PARENT:
					fprintf(fff, _("%sの召喚者が既にいないため消え去った。\n", "%s disappeared because there does not exist summoner.\n"), note);
					break;
				default:
					fprintf(fff, "\n");
					break;
			}
			break;
		}
		default:
			break;
	}

	my_fclose(fff);

	if (do_level) write_level = FALSE;

	return (0);
}


#define MAX_SUBTITLE (sizeof(subtitle)/sizeof(subtitle[0]))

/*!
 * @brief 日記のタイトル表記と内容出力 /
 * @return なし
 */
static void do_cmd_disp_nikki(void)
{
	char nikki_title[256];
	char file_name[80];
	char buf[1024];
	char tmp[80];
#ifdef JP
	static const char subtitle[][30] = {"最強の肉体を求めて",
					   "人生それははかない",
					   "明日に向かって",
					   "棚からぼたもち",
					   "あとの祭り",
					   "それはいい考えだ",
					   "何とでも言え",
					   "兎にも角にも",
					   "ウソだけど",
					   "もはやこれまで",
					   "なんでこうなるの",
					   "それは無理だ",
					   "倒すべき敵はゲ○ツ",
					   "ん～？聞こえんなぁ",
					   "オレの名を言ってみろ",
					   "頭が変になっちゃった",
					   "互換しません",
					   "せっかくだから",
					   "まだまだ甘いね",
					   "むごいむごすぎる",
					   "こんなもんじゃない",
					   "だめだこりゃ",
					   "次いってみよう",
					   "ちょっとだけよ",
					   "哀しき冒険者",
					   "野望の果て",
					   "無限地獄",
					   "神に喧嘩を売る者",
					   "未知の世界へ",
					   "最高の頭脳を求めて"};
#else
	static const char subtitle[][51] ={"Quest of The World's Toughest Body",
					   "Attack is the best form of defence.",
					   "Might is right.",
					   "An unexpected windfall",
					   "A drowning man will catch at a straw",
					   "Don't count your chickens before they are hatched.",
					   "It is no use crying over spilt milk.",
					   "Seeing is believing.",
					   "Strike the iron while it is hot.",
					   "I don't care what follows.",
					   "To dig a well to put out a house on fire.",
					   "Tomorrow is another day.",
					   "Easy come, easy go.",
					   "The more haste, the less speed.",
					   "Where there is life, there is hope.",
					   "There is no royal road to *WINNER*.",
					   "Danger past, God forgotten.",
					   "The best thing to do now is to run away.",
					   "Life is but an empty dream.",
					   "Dead men tell no tales.",
					   "A book that remains shut is but a block.",
					   "Misfortunes never come singly.",
					   "A little knowledge is a dangerous thing.",
					   "History repeats itself.",
					   "*WINNER* was not built in a day.",
					   "Ignorance is bliss.",
					   "To lose is to win?",
					   "No medicine can cure folly.",
					   "All good things come to an end.",
					   "M$ Empire strikes back.",
					   "To see is to believe",
					   "Time is money.",
					   "Quest of The World's Greatest Brain"};
#endif
	sprintf(file_name,_("playrecord-%s.txt", "playrec-%s.txt"),savefile_base);

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

	if (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_MONK || p_ptr->pclass == CLASS_SAMURAI || p_ptr->pclass == CLASS_BERSERKER)
		strcpy(tmp,subtitle[randint0(MAX_SUBTITLE-1)]);
	else if (p_ptr->pclass == CLASS_MAGE || p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER)
		strcpy(tmp,subtitle[randint0(MAX_SUBTITLE-1)+1]);
	else strcpy(tmp,subtitle[randint0(MAX_SUBTITLE-2)+1]);

#ifdef JP
	sprintf(nikki_title, "「%s%s%sの伝説 -%s-」",
		ap_ptr->title, ap_ptr->no ? "の" : "", player_name, tmp);
#else
	sprintf(nikki_title, "Legend of %s %s '%s'",
		ap_ptr->title, player_name, tmp);
#endif

	/* Display the file contents */
	show_file(FALSE, buf, nikki_title, -1, 0);
}

/*!
 * @brief 日記に任意の内容を表記するコマンドのメインルーチン /
 * @return なし
 */
static void do_cmd_bunshou(void)
{
	char tmp[80] = "\0";
	char bunshou[80] = "\0";

	if (get_string(_("内容: ", "diary note: "), tmp, 79))
	{
		strcpy(bunshou, tmp);

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, bunshou);
	}
}

/*!
 * @brief 最後に取得したアイテムの情報を日記に追加するメインルーチン /
 * @return なし
 */
static void do_cmd_last_get(void)
{
	char buf[256];
	s32b turn_tmp;

	if (record_o_name[0] == '\0') return;

	sprintf(buf,_("%sの入手を記録します。", "Do you really want to record getting %s? "),record_o_name);
	if (!get_check(buf)) return;

	turn_tmp = turn;
	turn = record_turn;
	sprintf(buf,_("%sを手に入れた。", "descover %s."), record_o_name);
	do_cmd_write_nikki(NIKKI_BUNSHOU, 0, buf);
	turn = turn_tmp;
}

static void do_cmd_erase_nikki(void)
{
	char file_name[80];
	char buf[256];
	FILE *fff = NULL;

	if (!get_check(_("本当に記録を消去しますか？", "Do you really want to delete all your record? "))) return;
		sprintf(file_name,_("playrecord-%s.txt", "playrec-%s.txt"),savefile_base);

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

	/* Remove the file */
	fd_kill(buf);

	fff = my_fopen(buf, "w");
	if(fff){
		my_fclose(fff);
		msg_format(_("記録を消去しました。", "deleted record."));
	}else{
		msg_format(_("%s の消去に失敗しました。", "failed to delete %s."), buf);
	}
	msg_print(NULL);
}

/*!
 * @brief 日記コマンド
 * @return なし
 */
void do_cmd_nikki(void)
{
	int i;

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save the screen */
	screen_save();

	/* Interact until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Ask for a choice */
		prt(_("[ 記録の設定 ]", "[ Play Record ]"), 2, 0);

		/* Give some choices */
		prt(_("(1) 記録を見る", "(1) Display your record"), 4, 5);
		prt(_("(2) 文章を記録する", "(2) Add record"), 5, 5);
		prt(_("(3) 直前に入手又は鑑定したものを記録する", "(3) Record item you last get/identify"), 6, 5);
		prt(_("(4) 記録を消去する", "(4) Delete your record"), 7, 5);

		prt(_("(R) プレイ動画を記録する/中止する", "(R) Record playing movie / or stop it"), 9, 5);

		/* Prompt */
		prt(_("コマンド:", "Command: "), 18, 0);

		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;

		switch (i)
		{
		case '1':
			do_cmd_disp_nikki();
			break;
		case '2':
			do_cmd_bunshou();
			break;
		case '3':
			do_cmd_last_get();
			break;
		case '4':
			do_cmd_erase_nikki();
			break;
		case 'r': case 'R':
			screen_load();
			prepare_movie_hooks();
			return;
		default: /* Unknown option */
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Restore the screen */
	screen_load();
}

/*!
 * @brief 画面を再描画するコマンドのメインルーチン
 * Hack -- redraw the screen
 * @return なし
 * @details
 * <pre>
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This command is also used to "instantiate" the results of the user
 * selecting various things, such as graphics mode, so it must call
 * the "TERM_XTRA_REACT" hook before redrawing the windows.
 * </pre>
 */
void do_cmd_redraw(void)
{
	int j;

	term *old = Term;


	/* Hack -- react to changes */
	Term_xtra(TERM_XTRA_REACT, 0);


	/* Combine and Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);


	/* Update torch */
	p_ptr->update |= (PU_TORCH);

	/* Update stuff */
	p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

	/* Forget lite/view */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update lite/view */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw everything */
	p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);

	/* Window stuff */
	p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

	update_playtime();

	/* Hack -- update */
	handle_stuff();

	if (p_ptr->prace == RACE_ANDROID) calc_android_exp();


	/* Redraw every window */
	for (j = 0; j < 8; j++)
	{
		/* Dead window */
		if (!angband_term[j]) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Redraw */
		Term_redraw();

		/* Refresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}


/*!
 * @brief 名前を変更するコマンドのメインルーチン
 * Hack -- change name
 * @return なし
 */
void do_cmd_change_name(void)
{
	char	c;

	int		mode = 0;

	char	tmp[160];


	/* Save the screen */
	screen_save();

	/* Forever */
	while (1)
	{
		update_playtime();

		/* Display the player */
		display_player(mode);

		if (mode == 4)
		{
			mode = 0;
			display_player(mode);
		}

		/* Prompt */
#ifdef JP
		Term_putstr(2, 23, -1, TERM_WHITE,
			    "['c'で名前変更, 'f'でファイルへ書出, 'h'でモード変更, ESCで終了]");
#else
		Term_putstr(2, 23, -1, TERM_WHITE,
			"['c' to change name, 'f' to file, 'h' to change mode, or ESC]");
#endif


		/* Query */
		c = inkey();

		/* Exit */
		if (c == ESCAPE) break;

		/* Change name */
		if (c == 'c')
		{
			get_name();

			/* Process the player name */
			process_player_name(FALSE);
		}

		/* File dump */
		else if (c == 'f')
		{
			sprintf(tmp, "%s.txt", player_base);
			if (get_string(_("ファイル名: ", "File name: "), tmp, 80))
			{
				if (tmp[0] && (tmp[0] != ' '))
				{
					file_character(tmp);
				}
			}
		}

		/* Toggle mode */
		else if (c == 'h')
		{
			mode++;
		}

		/* Oops */
		else
		{
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Restore the screen */
	screen_load();

	/* Redraw everything */
	p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

	handle_stuff();
}


/*!
 * @brief 最近表示されたメッセージを再表示するコマンドのメインルーチン
 * Recall the most recent message
 * @return なし
 */
void do_cmd_message_one(void)
{
	/* Recall one message XXX XXX XXX */
	prt(format("> %s", message_str(0)), 0, 0);
}


/*!
 * @brief メッセージのログを表示するコマンドのメインルーチン
 * Recall the most recent message
 * @return なし
 * @details
 * <pre>
 * Show previous messages to the user	-BEN-
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilite the matching portions of the string.
 * </pre>
 */
void do_cmd_messages(int num_now)
{
	int i, n;

	char shower_str[81];
	char finder_str[81];
	char back_str[81];
	cptr shower = NULL;
	int wid, hgt;
	int num_lines;

	/* Get size */
	Term_get_size(&wid, &hgt);

	/* Number of message lines in a screen */
	num_lines = hgt - 4;

	/* Wipe finder */
	strcpy(finder_str, "");

	/* Wipe shower */
	strcpy(shower_str, "");

	/* Total messages */
	n = message_num();

	/* Start on first message */
	i = 0;

	/* Save the screen */
	screen_save();

	/* Clear screen */
	Term_clear();

	/* Process requests until done */
	while (1)
	{
		int j;
		int skey;

		/* Dump up to 20 lines of messages */
		for (j = 0; (j < num_lines) && (i + j < n); j++)
		{
			cptr msg = message_str(i+j);

			/* Dump the messages, bottom to top */
			c_prt((i + j < num_now ? TERM_WHITE : TERM_SLATE), msg, num_lines + 1 - j, 0);

			/* Hilite "shower" */
			if (shower && shower[0])
			{
				cptr str = msg;

				/* Display matches */
				while ((str = my_strstr(str, shower)) != NULL)
				{
					int len = strlen(shower);

					/* Display the match */
					Term_putstr(str-msg, num_lines + 1 - j, len, TERM_YELLOW, shower);

					/* Advance */
					str += len;
				}
			}
		}

		/* Erase remaining lines */
		for (; j < num_lines; j++)
		{
			Term_erase(0, num_lines + 1 - j, 255);
		}

		/* Display header XXX XXX XXX */
#ifdef JP
		/* translation */
		prt(format("以前のメッセージ %d-%d 全部で(%d)",
			   i, i + j - 1, n), 0, 0);
#else
		prt(format("Message Recall (%d-%d of %d)",
			   i, i + j - 1, n), 0, 0);
#endif

		/* Display prompt (not very informative) */
		prt(_("[ 'p' で更に古いもの, 'n' で更に新しいもの, '/' で検索, ESC で中断 ]",
			  "[Press 'p' for older, 'n' for newer, ..., or ESCAPE]"), hgt - 1, 0);

		/* Get a command */
		skey = inkey_special(TRUE);

		/* Exit on Escape */
		if (skey == ESCAPE) break;

		/* Hack -- Save the old index */
		j = i;

		switch (skey)
		{
		/* Hack -- handle show */
		case '=':
			/* Prompt */
			prt(_("強調: ", "Show: "), hgt - 1, 0);

			/* Get a "shower" string, or continue */
			strcpy(back_str, shower_str);
			if (askfor(shower_str, 80))
			{
				/* Show it */
				shower = shower_str[0] ? shower_str : NULL;
			}
			else strcpy(shower_str, back_str);

			/* Okay */
			continue;

		/* Hack -- handle find */
		case '/':
		case KTRL('s'):
			{
				int z;

				/* Prompt */
				prt(_("検索: ", "Find: "), hgt - 1, 0);

				/* Get a "finder" string, or continue */
				strcpy(back_str, finder_str);
				if (!askfor(finder_str, 80))
				{
					strcpy(finder_str, back_str);
					continue;
				}
				else if (!finder_str[0])
				{
					shower = NULL; /* Stop showing */
					continue;
				}

				/* Show it */
				shower = finder_str;

				/* Scan messages */
				for (z = i + 1; z < n; z++)
				{
					cptr msg = message_str(z);

					/* Search for it */
					if (my_strstr(msg, finder_str))
					{
						/* New location */
						i = z;

						/* Done */
						break;
					}
				}
			}
			break;

		/* Recall 1 older message */
		case SKEY_TOP:
			/* Go to the oldest line */
			i = n - num_lines;
			break;

		/* Recall 1 newer message */
		case SKEY_BOTTOM:
			/* Go to the newest line */
			i = 0;
			break;

		/* Recall 1 older message */
		case '8':
		case SKEY_UP:
		case '\n':
		case '\r':
			/* Go older if legal */
			i = MIN(i + 1, n - num_lines);
			break;

		/* Recall 10 older messages */
		case '+':
			/* Go older if legal */
			i = MIN(i + 10, n - num_lines);
			break;

		/* Recall 20 older messages */
		case 'p':
		case KTRL('P'):
		case ' ':
		case SKEY_PGUP:
			/* Go older if legal */
			i = MIN(i + num_lines, n - num_lines);
			break;

		/* Recall 20 newer messages */
		case 'n':
		case KTRL('N'):
		case SKEY_PGDOWN:
			/* Go newer (if able) */
			i = MAX(0, i - num_lines);
			break;

		/* Recall 10 newer messages */
		case '-':
			/* Go newer (if able) */
			i = MAX(0, i - 10);
			break;

		/* Recall 1 newer messages */
		case '2':
		case SKEY_DOWN:
			/* Go newer (if able) */
			i = MAX(0, i - 1);
			break;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	screen_load();
}



/*!
 * チートオプションの最大数 / Number of cheating options
 */
#define CHEAT_MAX 7

/*!
 * チーとオプションの定義テーブル / Cheating options
 */
static option_type cheat_info[CHEAT_MAX] =
{
	{ &cheat_peek,		FALSE,	255,	0x01, 0x00,
		"cheat_peek",		_("アイテムの生成をのぞき見る", "Peek into object creation")
	},

	{ &cheat_hear,		FALSE,	255,	0x02, 0x00,
		"cheat_hear",		_("モンスターの生成をのぞき見る", "Peek into monster creation")
	},

	{ &cheat_room,		FALSE,	255,	0x04, 0x00,
		"cheat_room",		_("ダンジョンの生成をのぞき見る", "Peek into dungeon creation")
	},

	{ &cheat_xtra,		FALSE,	255,	0x08, 0x00,
		"cheat_xtra",		_("その他の事をのぞき見る", "Peek into something else")
	},

	{ &cheat_know,		FALSE,	255,	0x10, 0x00,
		"cheat_know",		_("完全なモンスターの思い出を知る", "Know complete monster info")
	},

	{ &cheat_live,		FALSE,	255,	0x20, 0x00,
		"cheat_live",		_("死を回避することを可能にする", "Allow player to avoid death")
	},

	{ &cheat_save,		FALSE,	255,	0x40, 0x00,
		"cheat_save",		_("死んだ時セーブするか確認する", "Ask for saving death")
	}
};

/*!
 * @brief チートオプションを変更するコマンドのメインルーチン
 * Interact with some options for cheating
 * @param info 表示メッセージ
 * @return なし
 */
static void do_cmd_options_cheat(cptr info)
{
	char	ch;

	int		i, k = 0, n = CHEAT_MAX;

	char	buf[80];


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		int dir;

		/* Prompt XXX XXX XXX */
		sprintf(buf, _("%s ( リターンで次へ, y/n でセット, ESC で決定 )", "%s (RET to advance, y/n to set, ESC to accept) "), info);

		prt(buf, 0, 0);

#ifdef JP
		/* 詐欺オプションをうっかりいじってしまう人がいるようなので注意 */
		prt("                                 <<  注意  >>", 11, 0);
		prt("      詐欺オプションを一度でも設定すると、スコア記録が残らなくなります！", 12, 0);
		prt("      後に解除してもダメですので、勝利者を目指す方はここのオプションはい", 13, 0);
		prt("      じらないようにして下さい。", 14, 0);
#endif
		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%s)",
			    cheat_info[i].o_desc,
			    (*cheat_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
			    cheat_info[i].o_text);
			c_prt(a, buf, i + 2, 0);
		}

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ch = inkey();

		/*
		 * HACK - Try to translate the key into a direction
		 * to allow using the roguelike keys for navigation.
		 */
		dir = get_keymap_dir(ch);
		if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8))
			ch = I2D(dir);

		/* Analyze */
		switch (ch)
		{
			case ESCAPE:
			{
				return;
			}

			case '-':
			case '8':
			{
				k = (n + k - 1) % n;
				break;
			}

			case ' ':
			case '\n':
			case '\r':
			case '2':
			{
				k = (k + 1) % n;
				break;
			}

			case 'y':
			case 'Y':
			case '6':
			{
				if(!p_ptr->noscore)
				do_cmd_write_nikki(NIKKI_BUNSHOU, 0,
							_("詐欺オプションをONにして、スコアを残せなくなった。", "give up sending score to use cheating options."));
				p_ptr->noscore |= (cheat_info[k].o_set * 256 + cheat_info[k].o_bit);
				(*cheat_info[k].o_var) = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case 'N':
			case '4':
			{
				(*cheat_info[k].o_var) = FALSE;
				k = (k + 1) % n;
				break;
			}

			case '?':
			{
				strnfmt(buf, sizeof(buf), _("joption.txt#%s", "option.txt#%s"), cheat_info[k].o_text);
				/* Peruse the help file */
				(void)show_file(TRUE, buf, NULL, 0, 0);

				Term_clear(); 
				break;
			}

			default:
			{
				bell();
				break;
			}
		}
	}
}


/*!
 * 自動セーブオプションテーブル
 */
static option_type autosave_info[2] =
{
	{ &autosave_l,      FALSE, 255, 0x01, 0x00,
	    "autosave_l",    _("新しい階に入る度に自動セーブする", "Autosave when entering new levels") },

	{ &autosave_t,      FALSE, 255, 0x02, 0x00,
	    "autosave_t",   _("一定ターン毎に自動セーブする", "Timed autosave") },
};

/*!
 * @brief セーブ頻度ターンの次の値を返す
 * @param current 現在のセーブ頻度ターン値
 * @return 次のセーブ頻度ターン値
 */
static s16b toggle_frequency(s16b current)
{
	switch (current)
	{
	case 0: return 50;
	case 50: return 100;
	case 100: return 250;
	case 250: return 500;
	case 500: return 1000;
	case 1000: return 2500;
	case 2500: return 5000;
	case 5000: return 10000;
	case 10000: return 25000;
	default: return 0;
	}
}


/*!
 * @brief 自動セーブオプションを変更するコマンドのメインルーチン
 * @param info 表示メッセージ
 * @return なし
 */
static void do_cmd_options_autosave(cptr info)
{
	char	ch;

	int     i, k = 0, n = 2;

	char	buf[80];


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		/* Prompt XXX XXX XXX */
		sprintf(buf, _("%s ( リターンで次へ, y/n でセット, F で頻度を入力, ESC で決定 ) ", 
					   "%s (RET to advance, y/n to set, 'F' for frequency, ESC to accept) "), info);

		prt(buf, 0, 0);

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%s)",
			    autosave_info[i].o_desc,
			    (*autosave_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
			    autosave_info[i].o_text);
			c_prt(a, buf, i + 2, 0);
		}
		prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"),  autosave_freq), 5, 0);

		/* Hilite current option */
		move_cursor(k + 2, 50);

		/* Get a key */
		ch = inkey();

		/* Analyze */
		switch (ch)
		{
			case ESCAPE:
			{
				return;
			}

			case '-':
			case '8':
			{
				k = (n + k - 1) % n;
				break;
			}

			case ' ':
			case '\n':
			case '\r':
			case '2':
			{
				k = (k + 1) % n;
				break;
			}

			case 'y':
			case 'Y':
			case '6':
			{

				(*autosave_info[k].o_var) = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case 'N':
			case '4':
			{
				(*autosave_info[k].o_var) = FALSE;
				k = (k + 1) % n;
				break;
			}

			case 'f':
			case 'F':
			{
				autosave_freq = toggle_frequency(autosave_freq);
				prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"), autosave_freq), 5, 0);
				break;
			}

			case '?':
			{
				(void)show_file(TRUE, _("joption.txt#Autosave", "option.txt#Autosave"), NULL, 0, 0);
				Term_clear(); 
				break;
			}

			default:
			{
				bell();
				break;
			}
		}
	}
}


/*!
 * @brief 標準オプションを変更するコマンドのサブルーチン /
 * Interact with some options
 * @param page オプションページ番号
 * @param info 表示メッセージ
 * @return なし
 */
void do_cmd_options_aux(int page, cptr info)
{
	char    ch;
	int     i, k = 0, n = 0, l;
	int     opt[24];
	char    buf[80];
	bool    browse_only = (page == OPT_PAGE_BIRTH) && character_generated &&
	                      (!p_ptr->wizard || !allow_debug_opts);


	/* Lookup the options */
	for (i = 0; i < 24; i++) opt[i] = 0;

	/* Scan the options */
	for (i = 0; option_info[i].o_desc; i++)
	{
		/* Notice options on this "page" */
		if (option_info[i].o_page == page) opt[n++] = i;
	}


	/* Clear screen */
	Term_clear();

	/* Interact with the player */
	while (TRUE)
	{
		int dir;

		/* Prompt XXX XXX XXX */
		sprintf(buf, _("%s (リターン:次, %sESC:終了, ?:ヘルプ) ", "%s (RET:next, %s, ?:help) "),
					info, browse_only ? _("", "ESC:exit") : _("y/n:変更, ", "y/n:change, ESC:accept"));
		prt(buf, 0, 0);

		/* HACK -- description for easy-auto-destroy options */
		if (page == OPT_PAGE_AUTODESTROY) 
			c_prt(TERM_YELLOW, _("以下のオプションは、簡易自動破壊を使用するときのみ有効", 
								 "Following options will protect items from easy auto-destroyer."), 6, _(6, 3));

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%.19s)",
				option_info[opt[i]].o_desc,
				(*option_info[opt[i]].o_var ? _("はい  ", "yes") : _("いいえ", "no ")),
				option_info[opt[i]].o_text);
			if ((page == OPT_PAGE_AUTODESTROY) && i > 2) c_prt(a, buf, i + 5, 0);
			else c_prt(a, buf, i + 2, 0);
		}

		if ((page == OPT_PAGE_AUTODESTROY) && (k > 2)) l = 3;
		else l = 0;

		/* Hilite current option */
		move_cursor(k + 2 + l, 50);

		/* Get a key */
		ch = inkey();

		/*
		 * HACK - Try to translate the key into a direction
		 * to allow using the roguelike keys for navigation.
		 */
		dir = get_keymap_dir(ch);
		if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8))
			ch = I2D(dir);

		/* Analyze */
		switch (ch)
		{
			case ESCAPE:
			{
				return;
			}

			case '-':
			case '8':
			{
				k = (n + k - 1) % n;
				break;
			}

			case ' ':
			case '\n':
			case '\r':
			case '2':
			{
				k = (k + 1) % n;
				break;
			}

			case 'y':
			case 'Y':
			case '6':
			{
				if (browse_only) break;
				(*option_info[opt[k]].o_var) = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case 'N':
			case '4':
			{
				if (browse_only) break;
				(*option_info[opt[k]].o_var) = FALSE;
				k = (k + 1) % n;
				break;
			}

			case 't':
			case 'T':
			{
				if (!browse_only) (*option_info[opt[k]].o_var) = !(*option_info[opt[k]].o_var);
				break;
			}

			case '?':
			{
				strnfmt(buf, sizeof(buf), _("joption.txt#%s", "option.txt#%s"), option_info[opt[k]].o_text);
				/* Peruse the help file */
				(void)show_file(TRUE, buf, NULL, 0, 0);

				Term_clear();
				break;
			}

			default:
			{
				bell();
				break;
			}
		}
	}
}


/*!
 * @brief ウィンドウオプションを変更するコマンドのメインルーチン /
 * Modify the "window" options
 * @return なし
 */
static void do_cmd_options_win(void)
{
	int i, j, d;

	int y = 0;
	int x = 0;

	char ch;

	bool go = TRUE;

	u32b old_flag[8];


	/* Memorize old flags */
	for (j = 0; j < 8; j++)
	{
		/* Acquire current flags */
		old_flag[j] = window_flag[j];
	}


	/* Clear screen */
	Term_clear();

	/* Interact */
	while (go)
	{
		/* Prompt XXX XXX XXX */
		prt(_("ウィンドウ・フラグ (<方向>で移動, tでチェンジ, y/n でセット, ESC)", "Window Flags (<dir>, t, y, n, ESC) "), 0, 0);

		/* Display the windows */
		for (j = 0; j < 8; j++)
		{
			byte a = TERM_WHITE;

			cptr s = angband_term_name[j];

			/* Use color */
			if (j == x) a = TERM_L_BLUE;

			/* Window name, staggered, centered */
			Term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
		}

		/* Display the options */
		for (i = 0; i < 16; i++)
		{
			byte a = TERM_WHITE;

			cptr str = window_flag_desc[i];

			/* Use color */
			if (i == y) a = TERM_L_BLUE;

			/* Unused option */
			if (!str) str = _("(未使用)", "(Unused option)");

			/* Flag name */
			Term_putstr(0, i + 5, -1, a, str);

			/* Display the windows */
			for (j = 0; j < 8; j++)
			{
				byte a = TERM_WHITE;

				char c = '.';

				/* Use color */
				if ((i == y) && (j == x)) a = TERM_L_BLUE;

				/* Active flag */
				if (window_flag[j] & (1L << i)) c = 'X';

				/* Flag value */
				Term_putch(35 + j * 5, i + 5, a, c);
			}
		}

		/* Place Cursor */
		Term_gotoxy(35 + x * 5, y + 5);

		/* Get key */
		ch = inkey();

		/* Analyze */
		switch (ch)
		{
			case ESCAPE:
			{
				go = FALSE;
				break;
			}

			case 'T':
			case 't':
			{
				/* Clear windows */
				for (j = 0; j < 8; j++)
				{
					window_flag[j] &= ~(1L << y);
				}

				/* Clear flags */
				for (i = 0; i < 16; i++)
				{
					window_flag[x] &= ~(1L << i);
				}

				/* Fall through */
			}

			case 'y':
			case 'Y':
			{
				/* Ignore screen */
				if (x == 0) break;

				/* Set flag */
				window_flag[x] |= (1L << y);
				break;
			}

			case 'n':
			case 'N':
			{
				/* Clear flag */
				window_flag[x] &= ~(1L << y);
				break;
			}

			case '?':
			{
				(void)show_file(TRUE, _("joption.txt#Window", "option.txt#Window"), NULL, 0, 0);

				Term_clear(); 
				break;
			}

			default:
			{
				d = get_keymap_dir(ch);

				x = (x + ddx[d] + 8) % 8;
				y = (y + ddy[d] + 16) % 16;

				if (!d) bell();
			}
		}
	}

	/* Notice changes */
	for (j = 0; j < 8; j++)
	{
		term *old = Term;

		/* Dead window */
		if (!angband_term[j]) continue;

		/* Ignore non-changes */
		if (window_flag[j] == old_flag[j]) continue;

		/* Activate */
		Term_activate(angband_term[j]);

		/* Erase */
		Term_clear();

		/* Refresh */
		Term_fresh();

		/* Restore */
		Term_activate(old);
	}
}



#define OPT_NUM 15

static struct opts
{
	char key;
	cptr name;
	int row;
}
option_fields[OPT_NUM] =
{
#ifdef JP
	{ '1', "    キー入力     オプション", 3 },
	{ '2', "   マップ画面    オプション", 4 },
	{ '3', "  テキスト表示   オプション", 5 },
	{ '4', "  ゲームプレイ   オプション", 6 },
	{ '5', "  行動中止関係   オプション", 7 },
	{ '6', "  簡易自動破壊   オプション", 8 },
	{ 'r', "   プレイ記録    オプション", 9 },

	{ 'p', "自動拾いエディタ", 11 },
	{ 'd', " 基本ウェイト量 ", 12 },
	{ 'h', "低ヒットポイント", 13 },
	{ 'm', "  低魔力色閾値  ", 14 },
	{ 'a', "   自動セーブ    オプション", 15 },
	{ 'w', "ウインドウフラグ", 16 },

	{ 'b', "      初期       オプション (参照のみ)", 18 },
	{ 'c', "      詐欺       オプション", 19 },
#else
	{ '1', "Input Options", 3 },
	{ '2', "Map Screen Options", 4 },
	{ '3', "Text Display Options", 5 },
	{ '4', "Game-Play Options", 6 },
	{ '5', "Disturbance Options", 7 },
	{ '6', "Easy Auto-Destroyer Options", 8 },
	{ 'r', "Play record Options", 9 },

	{ 'p', "Auto-picker/destroyer editor", 11 },
	{ 'd', "Base Delay Factor", 12 },
	{ 'h', "Hitpoint Warning", 13 },
	{ 'm', "Mana Color Threshold", 14 },
	{ 'a', "Autosave Options", 15 },
	{ 'w', "Window Flags", 16 },

	{ 'b', "Birth Options (Browse Only)", 18 },
	{ 'c', "Cheat Options", 19 },
#endif
};


/*!
 * @brief 標準オプションを変更するコマンドのメインルーチン /
 * Set or unset various options.
 * @return なし
 * @details
 * <pre>
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 * </pre>
 */
void do_cmd_options(void)
{
	char k;
	int i, d, skey;
	int y = 0;

	/* Save the screen */
	screen_save();

	/* Interact */
	while (1)
	{
		int n = OPT_NUM;

		/* Does not list cheat option when cheat option is off */
		if (!p_ptr->noscore && !allow_debug_opts) n--;

		/* Clear screen */
		Term_clear();

		/* Why are we here */
		prt(_("[ オプションの設定 ]", "TinyAngband options"), 1, 0);

		while(1)
		{
			/* Give some choices */
			for (i = 0; i < n; i++)
			{
				byte a = TERM_WHITE;
				if (i == y) a = TERM_L_BLUE;
				Term_putstr(5, option_fields[i].row, -1, a, 
					format("(%c) %s", toupper(option_fields[i].key), option_fields[i].name));
			}

			prt(_("<方向>で移動, Enterで決定, ESCでキャンセル, ?でヘルプ: ", "Move to <dir>, Select to Enter, Cancel to ESC, ? to help: "), 21, 0);

			/* Get command */
			skey = inkey_special(TRUE);
			if (!(skey & SKEY_MASK)) k = (char)skey;
			else k = 0;

			/* Exit */
			if (k == ESCAPE) break;

			if (my_strchr("\n\r ", k))
			{
				k = option_fields[y].key;
				break;
			}

			for (i = 0; i < n; i++)
			{
				if (tolower(k) == option_fields[i].key) break;
			}

			/* Command is found */
			if (i < n) break;

			/* Hack -- browse help */
			if (k == '?') break;

			/* Move cursor */
			d = 0;
			if (skey == SKEY_UP) d = 8;
			if (skey == SKEY_DOWN) d = 2;
			y = (y + ddy[d] + n) % n;
			if (!d) bell();
		}

		/* Exit */
		if (k == ESCAPE) break;

		/* Analyze */
		switch (k)
		{
			case '1':
			{
				/* Process the general options */
				do_cmd_options_aux(OPT_PAGE_INPUT, _("キー入力オプション", "Input Options"));
				break;
			}

			case '2':
			{
				/* Process the general options */
				do_cmd_options_aux(OPT_PAGE_MAPSCREEN, _("マップ画面オプション", "Map Screen Options"));
				break;
			}

			case '3':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_TEXT, _("テキスト表示オプション", "Text Display Options"));
				break;
			}

			case '4':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_GAMEPLAY, _("ゲームプレイ・オプション", "Game-Play Options"));
				break;
			}

			case '5':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_DISTURBANCE, _("行動中止関係のオプション", "Disturbance Options"));
				break;
			}

			case '6':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_AUTODESTROY, _("簡易自動破壊オプション", "Easy Auto-Destroyer Options"));
				break;
			}

			/* Play-record Options */
			case 'R':
			case 'r':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_PLAYRECORD, _("プレイ記録オプション", "Play-record Options"));
				break;
			}

			/* Birth Options */
			case 'B':
			case 'b':
			{
				/* Spawn */
				do_cmd_options_aux(OPT_PAGE_BIRTH, (!p_ptr->wizard || !allow_debug_opts) ? 
							_("初期オプション(参照のみ)", "Birth Options(browse only)") : 
							_("初期オプション((*)はスコアに影響)", "Birth Options((*)s effect score)"));
				break;
			}

			/* Cheating Options */
			case 'C':
			{
				if (!p_ptr->noscore && !allow_debug_opts)
				{
					/* Cheat options are not permitted */
					bell();
					break;
				}

				/* Spawn */
				do_cmd_options_cheat(_("詐欺師は決して勝利できない！", "Cheaters never win"));
				break;
			}

			case 'a':
			case 'A':
			{
				do_cmd_options_autosave(_("自動セーブ", "Autosave"));
				break;
			}

			/* Window flags */
			case 'W':
			case 'w':
			{
				/* Spawn */
				do_cmd_options_win();
				p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL |
						  PW_PLAYER | PW_MESSAGE | PW_OVERHEAD |
							PW_MONSTER | PW_OBJECT | PW_SNAPSHOT |
							PW_BORG_1 | PW_BORG_2 | PW_DUNGEON   |
							PW_MONSTER_LIST);
				break;
			}

			/* Auto-picker/destroyer editor */
			case 'P':
			case 'p':
			{
				do_cmd_edit_autopick();
				break;
			}

			/* Hack -- Delay Speed */
			case 'D':
			case 'd':
			{
				/* Prompt */
				clear_from(18);
				prt(_("コマンド: 基本ウェイト量", "Command: Base Delay Factor"), 19, 0);

				/* Get a new value */
				while (1)
				{
					int msec = delay_factor * delay_factor * delay_factor;
					prt(format(_("現在のウェイト: %d (%dミリ秒)", "Current base delay factor: %d (%d msec)"), delay_factor, msec), 22, 0);
					prt(_("ウェイト (0-9) ESCで決定: ", "Delay Factor (0-9 or ESC to accept): "), 20, 0);
					k = inkey();
					if (k == ESCAPE) break;
					else if (k == '?')
					{
						(void)show_file(TRUE, _("joption.txt#BaseDelay", "option.txt#BaseDelay"), NULL, 0, 0);
						Term_clear(); 
					}
					else if (isdigit(k)) delay_factor = D2I(k);
					else bell();
				}

				break;
			}

			/* Hack -- hitpoint warning factor */
			case 'H':
			case 'h':
			{
				/* Prompt */
				clear_from(18);
				prt(_("コマンド: 低ヒットポイント警告", "Command: Hitpoint Warning"), 19, 0);

				/* Get a new value */
				while (1)
				{
					prt(format(_("現在の低ヒットポイント警告: %d0%%", "Current hitpoint warning: %d0%%"), hitpoint_warn), 22, 0);
					prt(_("低ヒットポイント警告 (0-9) ESCで決定: ", "Hitpoint Warning (0-9 or ESC to accept): "), 20, 0);
					k = inkey();
					if (k == ESCAPE) break;
					else if (k == '?')
					{
						(void)show_file(TRUE, _("joption.txt#Hitpoint", "option.txt#Hitpoint"), NULL, 0, 0);
						Term_clear(); 
					}
					else if (isdigit(k)) hitpoint_warn = D2I(k);
					else bell();
				}

				break;
			}

			/* Hack -- mana color factor */
			case 'M':
			case 'm':
			{
				/* Prompt */
				clear_from(18);
				prt(_("コマンド: 低魔力色閾値", "Command: Mana Color Threshold"), 19, 0);
				
				/* Get a new value */
				while (1)
				{
					prt(format(_("現在の低魔力色閾値: %d0%%", "Current mana color threshold: %d0%%"), mana_warn), 22, 0);
					prt(_("低魔力閾値 (0-9) ESCで決定: ", "Mana color Threshold (0-9 or ESC to accept): "), 20, 0);
					k = inkey();
					if (k == ESCAPE) break;
					else if (k == '?')
					{
						(void)show_file(TRUE, _("joption.txt#Manapoint", "option.txt#Manapoint"), NULL, 0, 0);
						Term_clear(); 
					}
					else if (isdigit(k)) mana_warn = D2I(k);
					else bell();
				}

				break;
			}

			case '?':
				(void)show_file(TRUE, _("joption.txt", "option.txt"), NULL, 0, 0);
				Term_clear(); 
				break;

			/* Unknown option */
			default:
			{
				/* Oops */
				bell();
				break;
			}
		}

		/* Flush messages */
		msg_print(NULL);
	}


	/* Restore the screen */
	screen_load();

	/* Hack - Redraw equippy chars */
	p_ptr->redraw |= (PR_EQUIPPY);
}



/*!
 * @brief prefファイルを選択して処理する /
 * Ask for a "user pref line" and process it
 * @return なし
 * @details
 * XXX XXX XXX Allow absolute file names?
 */
void do_cmd_pref(void)
{
	char buf[80];

	/* Default */
	strcpy(buf, "");

	/* Ask for a "user pref command" */
	if (!get_string(_("設定変更コマンド: ", "Pref: "), buf, 80)) return;

	/* Process that pref command */
	(void)process_pref_file_command(buf);
}

/*!
 * @brief 自動拾い設定ファイルをロードするコマンドのメインルーチン /
 * @return なし
 */
void do_cmd_reload_autopick(void)
{
	if (!get_check(_("自動拾い設定ファイルをロードしますか? ", "Reload auto-pick preference file? "))) return;
	/* Load the file with messages */
	autopick_load_pref(TRUE);
}

#ifdef ALLOW_MACROS

/*!
 * @brief マクロ情報をprefファイルに保存する /
 * @param fname ファイル名
 * @return なし
 */
static errr macro_dump(cptr fname)
{
	static cptr mark = "Macro Dump";

	int i;

	char buf[1024];

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	if (!open_auto_dump(buf, mark)) return (-1);

	/* Start dumping */
	auto_dump_printf(_("\n# 自動マクロセーブ\n\n", "\n# Automatic macro dump\n\n"));
	
	/* Dump them */
	for (i = 0; i < macro__num; i++)
	{
		/* Extract the action */
		ascii_to_text(buf, macro__act[i]);

		/* Dump the macro */
		auto_dump_printf("A:%s\n", buf);

		/* Extract the action */
		ascii_to_text(buf, macro__pat[i]);

		/* Dump normal macros */
		auto_dump_printf("P:%s\n", buf);

		/* End the macro */
		auto_dump_printf("\n");
	}

	/* Close */
	close_auto_dump();

	/* Success */
	return (0);
}


/*!
 * @brief マクロのトリガーキーを取得する /
 * Hack -- ask for a "trigger" (see below)
 * @param buf キー表記を保管するバッファ
 * @return なし
 * @details
 * <pre>
 * Note the complex use of the "inkey()" function from "util.c".
 *
 * Note that both "flush()" calls are extremely important.
 * </pre>
 */
static void do_cmd_macro_aux(char *buf)
{
	int i, n = 0;

	char tmp[1024];


	/* Flush */
	flush();

	/* Do not process macros */
	inkey_base = TRUE;

	/* First key */
	i = inkey();

	/* Read the pattern */
	while (i)
	{
		/* Save the key */
		buf[n++] = i;

		/* Do not process macros */
		inkey_base = TRUE;

		/* Do not wait for keys */
		inkey_scan = TRUE;

		/* Attempt to read a key */
		i = inkey();
	}

	/* Terminate */
	buf[n] = '\0';

	/* Flush */
	flush();


	/* Convert the trigger */
	ascii_to_text(tmp, buf);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);
}

#endif

/*!
 * @brief マクロのキー表記からアスキーコードを得てターミナルに表示する /
 * Hack -- ask for a keymap "trigger" (see below)
 * @param buf キー表記を取得するバッファ
 * @return なし
 * @details
 * <pre>
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
 * </pre>
 */
static void do_cmd_macro_aux_keymap(char *buf)
{
	char tmp[1024];


	/* Flush */
	flush();


	/* Get a key */
	buf[0] = inkey();
	buf[1] = '\0';


	/* Convert to ascii */
	ascii_to_text(tmp, buf);

	/* Hack -- display the trigger */
	Term_addstr(-1, TERM_WHITE, tmp);


	/* Flush */
	flush();
}


/*!
 * @brief キーマップをprefファイルにダンプする /
 * Hack -- append all keymaps to the given file
 * @param fname ファイルネーム
 * @return エラーコード
 * @details
 */
static errr keymap_dump(cptr fname)
{
	static cptr mark = "Keymap Dump";
	int i;

	char key[1024];
	char buf[1024];

	int mode;

	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	if (!open_auto_dump(buf, mark)) return -1;

	/* Start dumping */
	auto_dump_printf(_("\n# 自動キー配置セーブ\n\n", "\n# Automatic keymap dump\n\n"));
	
	/* Dump them */
	for (i = 0; i < 256; i++)
	{
		cptr act;

		/* Loop up the keymap */
		act = keymap_act[mode][i];

		/* Skip empty keymaps */
		if (!act) continue;

		/* Encode the key */
		buf[0] = i;
		buf[1] = '\0';
		ascii_to_text(key, buf);

		/* Encode the action */
		ascii_to_text(buf, act);

		/* Dump the macro */
		auto_dump_printf("A:%s\n", buf);
		auto_dump_printf("C:%d:%s\n", mode, key);
	}

	/* Close */
	close_auto_dump();

	/* Success */
	return (0);
}


/*!
 * @brief マクロを設定するコマンドのメインルーチン /
 * Interact with "macros"
 * @return なし
 * @details
 * <pre>
 * Note that the macro "action" must be defined before the trigger.
 *
 * Could use some helpful instructions on this page.  XXX XXX XXX
 * </pre>
 */
void do_cmd_macros(void)
{
	int i;

	char tmp[1024];

	char buf[1024];

	int mode;


	/* Roguelike */
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}

	/* Original */
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);


	/* Save screen */
	screen_save();


	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Describe */
		prt(_("[ マクロの設定 ]", "Interact with Macros"), 2, 0);

		/* Describe that action */
		prt(_("マクロ行動が(もしあれば)下に表示されます:", "Current action (if any) shown below:"), 20, 0);

		/* Analyze the current action */
		ascii_to_text(buf, macro__buf);

		/* Display the current action */
		prt(buf, 22, 0);


		/* Selections */
		prt(_("(1) ユーザー設定ファイルのロード", "(1) Load a user pref file"), 4, 5);
#ifdef ALLOW_MACROS
		prt(_("(2) ファイルにマクロを追加", "(2) Append macros to a file"), 5, 5);
		prt(_("(3) マクロの確認", "(3) Query a macro"), 6, 5);
		prt(_("(4) マクロの作成", "(4) Create a macro"), 7, 5);
		prt(_("(5) マクロの削除", "(5) Remove a macro"), 8, 5);
		prt(_("(6) ファイルにキー配置を追加", "(6) Append keymaps to a file"), 9, 5);
		prt(_("(7) キー配置の確認", "(7) Query a keymap"), 10, 5);
		prt(_("(8) キー配置の作成", "(8) Create a keymap"), 11, 5);
		prt(_("(9) キー配置の削除", "(9) Remove a keymap"), 12, 5);
		prt(_("(0) マクロ行動の入力", "(0) Enter a new action"), 13, 5);
#endif /* ALLOW_MACROS */

		/* Prompt */
		prt(_("コマンド: ", "Command: "), 16, 0);

		/* Get a command */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Load a 'macro' file */
		else if (i == '1')
		{
			errr err;

			/* Prompt */
			prt(_("コマンド: ユーザー設定ファイルのロード", "Command: Load a user pref file"), 16, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 18, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Ask for a file */
			if (!askfor(tmp, 80)) continue;

			/* Process the given filename */
			err = process_pref_file(tmp);
			if (-2 == err)
			{
				msg_format(_("標準の設定ファイル'%s'を読み込みました。", "Loaded default '%s'."), tmp);
			}
			else if (err)
			{
				/* Prompt */
				msg_format(_("'%s'の読み込みに失敗しました！", "Failed to load '%s'!"), tmp);
			}
			else
			{
				msg_format(_("'%s'を読み込みました。", "Loaded '%s'."), tmp);
			}
		}

#ifdef ALLOW_MACROS

		/* Save macros */
		else if (i == '2')
		{
			/* Prompt */
			prt(_("コマンド: マクロをファイルに追加する", "Command: Append macros to a file"), 16, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 18, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Ask for a file */
			if (!askfor(tmp, 80)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);

			/* Prompt */
			msg_print(_("マクロを追加しました。", "Appended macros."));
		}

		/* Query a macro */
		else if (i == '3')
		{
			int k;

			/* Prompt */
			prt(_("コマンド: マクロの確認", "Command: Query a macro"), 16, 0);


			/* Prompt */
			prt(_("トリガーキー: ", "Trigger: "), 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Acquire action */
			k = macro_find_exact(buf);

			/* Nothing found */
			if (k < 0)
			{
				/* Prompt */
				msg_print(_("そのキーにはマクロは定義されていません。", "Found no macro."));
			}

			/* Found one */
			else
			{
				/* Obtain the action */
				strcpy(macro__buf, macro__act[k]);

				/* Analyze the current action */
				ascii_to_text(buf, macro__buf);

				/* Display the current action */
				prt(buf, 22, 0);

				/* Prompt */
				msg_print(_("マクロを確認しました。", "Found a macro."));
			}
		}

		/* Create a macro */
		else if (i == '4')
		{
			/* Prompt */
			prt(_("コマンド: マクロの作成", "Command: Create a macro"), 16, 0);

			/* Prompt */
			prt(_("トリガーキー: ", "Trigger: "), 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Clear */
			clear_from(20);

			/* Help message */
			c_prt(TERM_L_RED, _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
								"Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."), 22, 0);

			/* Prompt */
			prt(_("マクロ行動: ", "Action: "), 20, 0);

			/* Convert to text */
			ascii_to_text(tmp, macro__buf);

			/* Get an encoded action */
			if (askfor(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro__buf, tmp);

				/* Link the macro */
				macro_add(buf, macro__buf);

				/* Prompt */
				msg_print(_("マクロを追加しました。", "Added a macro."));
			}
		}

		/* Remove a macro */
		else if (i == '5')
		{
			/* Prompt */
			prt(_("コマンド: マクロの削除", "Command: Remove a macro"), 16, 0);

			/* Prompt */
			prt(_("トリガーキー: ", "Trigger: "), 18, 0);

			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Link the macro */
			macro_add(buf, buf);

			/* Prompt */
			msg_print(_("マクロを削除しました。", "Removed a macro."));
		}

		/* Save keymaps */
		else if (i == '6')
		{
			/* Prompt */
			prt(_("コマンド: キー配置をファイルに追加する", "Command: Append keymaps to a file"), 16, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 18, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Ask for a file */
			if (!askfor(tmp, 80)) continue;

			/* Dump the macros */
			(void)keymap_dump(tmp);

			/* Prompt */
			msg_print(_("キー配置を追加しました。", "Appended keymaps."));
		}

		/* Query a keymap */
		else if (i == '7')
		{
			cptr act;

			/* Prompt */
			prt(_("コマンド: キー配置の確認", "Command: Query a keymap"), 16, 0);

			/* Prompt */
			prt(_("押すキー: ", "Keypress: "), 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Look up the keymap */
			act = keymap_act[mode][(byte)(buf[0])];

			/* Nothing found */
			if (!act)
			{
				/* Prompt */
				msg_print(_("キー配置は定義されていません。", "Found no keymap."));
			}

			/* Found one */
			else
			{
				/* Obtain the action */
				strcpy(macro__buf, act);

				/* Analyze the current action */
				ascii_to_text(buf, macro__buf);

				/* Display the current action */
				prt(buf, 22, 0);

				/* Prompt */
				msg_print(_("キー配置を確認しました。", "Found a keymap."));
			}
		}

		/* Create a keymap */
		else if (i == '8')
		{
			/* Prompt */
			prt(_("コマンド: キー配置の作成", "Command: Create a keymap"), 16, 0);

			/* Prompt */
			prt(_("押すキー: ", "Keypress: "), 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Clear */
			clear_from(20);

			/* Help message */
			c_prt(TERM_L_RED, _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
							    "Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."), 22, 0);

			/* Prompt */
			prt(_("行動: ", "Action: "), 20, 0);

			/* Convert to text */
			ascii_to_text(tmp, macro__buf);

			/* Get an encoded action */
			if (askfor(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro__buf, tmp);

				/* Free old keymap */
				string_free(keymap_act[mode][(byte)(buf[0])]);

				/* Make new keymap */
				keymap_act[mode][(byte)(buf[0])] = string_make(macro__buf);

				/* Prompt */
				msg_print(_("キー配置を追加しました。", "Added a keymap."));
			}
		}

		/* Remove a keymap */
		else if (i == '9')
		{
			/* Prompt */
			prt(_("コマンド: キー配置の削除", "Command: Remove a keymap"), 16, 0);

			/* Prompt */
			prt(_("押すキー: ", "Keypress: "), 18, 0);

			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Free old keymap */
			string_free(keymap_act[mode][(byte)(buf[0])]);

			/* Make new keymap */
			keymap_act[mode][(byte)(buf[0])] = NULL;

			/* Prompt */
			msg_print(_("キー配置を削除しました。", "Removed a keymap."));
		}

		/* Enter a new action */
		else if (i == '0')
		{
			/* Prompt */
			prt(_("コマンド: マクロ行動の入力", "Command: Enter a new action"), 16, 0);

			/* Clear */
			clear_from(20);

			/* Help message */
			c_prt(TERM_L_RED, _("カーソルキーの左右でカーソル位置を移動。BackspaceかDeleteで一文字削除。",
								"Press Left/Right arrow keys to move cursor. Backspace/Delete to delete a char."), 22, 0);

			/* Prompt */
			prt(_("マクロ行動: ", "Action: "), 20, 0);

			/* Hack -- limit the value */
			tmp[80] = '\0';

			/* Get an encoded action */
			if (!askfor(buf, 80)) continue;

			/* Extract an action */
			text_to_ascii(macro__buf, buf);
		}

#endif /* ALLOW_MACROS */

		/* Oops */
		else
		{
			/* Oops */
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Load screen */
	screen_load();
}


static cptr lighting_level_str[F_LIT_MAX] =
{
#ifdef JP
	"標準色",
	"明色",
	"暗色",
#else
	"standard",
	"brightly lit",
	"darkened",
#endif
};


static bool cmd_visuals_aux(int i, int *num, int max)
{
	if (iscntrl(i))
	{
		char str[10] = "";
		int tmp;

		sprintf(str, "%d", *num);

		if (!get_string(format("Input new number(0-%d): ", max-1), str, 4))
			return FALSE;

		tmp = strtol(str, NULL, 0);
		if (tmp >= 0 && tmp < max)
			*num = tmp;
	}
	else if (isupper(i))
		*num = (*num + max - 1) % max;
	else
		*num = (*num + 1) % max;

	return TRUE;
}

static void print_visuals_menu(cptr choice_msg)
{
	prt(_("[ 画面表示の設定 ]", "Interact with Visuals"), 1, 0);
	
	/* Give some choices */
	prt(_("(0) ユーザー設定ファイルのロード", "(0) Load a user pref file"), 3, 5);
	
#ifdef ALLOW_VISUALS
	prt(_("(1) モンスターの 色/文字 をファイルに書き出す", "(1) Dump monster attr/chars"), 4, 5);
	prt(_("(2) アイテムの   色/文字 をファイルに書き出す", "(2) Dump object attr/chars"), 5, 5);
	prt(_("(3) 地形の       色/文字 をファイルに書き出す", "(3) Dump feature attr/chars"), 6, 5);
	prt(_("(4) モンスターの 色/文字 を変更する (数値操作)", "(4) Change monster attr/chars (numeric operation)"), 7, 5);
	prt(_("(5) アイテムの   色/文字 を変更する (数値操作)", "(5) Change object attr/chars (numeric operation)"), 8, 5);
	prt(_("(6) 地形の       色/文字 を変更する (数値操作)", "(6) Change feature attr/chars (numeric operation)"), 9, 5);
	prt(_("(7) モンスターの 色/文字 を変更する (シンボルエディタ)", "(7) Change monster attr/chars (visual mode)"), 10, 5);
	prt(_("(8) アイテムの   色/文字 を変更する (シンボルエディタ)", "(8) Change object attr/chars (visual mode)"), 11, 5);
	prt(_("(9) 地形の       色/文字 を変更する (シンボルエディタ)", "(9) Change feature attr/chars (visual mode)"), 12, 5);
#endif /* ALLOW_VISUALS */

	prt(_("(R) 画面表示方法の初期化", "(R) Reset visuals"), 13, 5);

	/* Prompt */
	prt(format("コマンド: %s", choice_msg ? choice_msg : _("", "")), 15, 0);
}

static void do_cmd_knowledge_monsters(bool *need_redraw, bool visual_only, int direct_r_idx);
static void do_cmd_knowledge_objects(bool *need_redraw, bool visual_only, int direct_k_idx);
static void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, int direct_f_idx, int *lighting_level);

/*
 * Interact with "visuals"
 */
void do_cmd_visuals(void)
{
	int i;
	char tmp[160];
	char buf[1024];
	bool need_redraw = FALSE;
	const char *empty_symbol = "<< ? >>";

	if (use_bigtile) empty_symbol = "<< ?? >>";

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save the screen */
	screen_save();

	/* Interact until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Ask for a choice */
		print_visuals_menu(NULL);

		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;

		switch (i)
		{
		/* Load a 'pref' file */
		case '0':
			/* Prompt */
			prt(_("コマンド: ユーザー設定ファイルのロード", "Command: Load a user pref file"), 15, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 17, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Query */
			if (!askfor(tmp, 70)) continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);

			need_redraw = TRUE;
			break;

#ifdef ALLOW_VISUALS

		/* Dump monster attr/chars */
		case '1':
		{
			static cptr mark = "Monster attr/chars";

			/* Prompt */
			prt(_("コマンド: モンスターの[色/文字]をファイルに書き出します", "Command: Dump monster attr/chars"), 15, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 17, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Get a filename */
			if (!askfor(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
			auto_dump_printf(_("\n# モンスターの[色/文字]の設定\n\n", "\n# Monster attr/char definitions\n\n"));

			/* Dump monsters */
			for (i = 0; i < max_r_idx; i++)
			{
				monster_race *r_ptr = &r_info[i];

				/* Skip non-entries */
				if (!r_ptr->name) continue;

				/* Dump a comment */
				auto_dump_printf("# %s\n", (r_name + r_ptr->name));

				/* Dump the monster attr/char info */
				auto_dump_printf("R:%d:0x%02X/0x%02X\n\n", i,
					(byte)(r_ptr->x_attr), (byte)(r_ptr->x_char));
			}

			/* Close */
			close_auto_dump();

			/* Message */
			msg_print(_("モンスターの[色/文字]をファイルに書き出しました。", "Dumped monster attr/chars."));

			break;
		}

		/* Dump object attr/chars */
		case '2':
		{
			static cptr mark = "Object attr/chars";

			/* Prompt */
			prt(_("コマンド: アイテムの[色/文字]をファイルに書き出します", "Command: Dump object attr/chars"), 15, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 17, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Get a filename */
			if (!askfor(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
			auto_dump_printf(_("\n# アイテムの[色/文字]の設定\n\n", "\n# Object attr/char definitions\n\n"));

			/* Dump objects */
			for (i = 0; i < max_k_idx; i++)
			{
				char o_name[80];
				object_kind *k_ptr = &k_info[i];

				/* Skip non-entries */
				if (!k_ptr->name) continue;

				if (!k_ptr->flavor)
				{
					/* Tidy name */
					strip_name(o_name, i);
				}
				else
				{
					object_type forge;

					/* Prepare dummy object */
					object_prep(&forge, i);

					/* Get un-shuffled flavor name */
					object_desc(o_name, &forge, OD_FORCE_FLAVOR);
				}

				/* Dump a comment */
				auto_dump_printf("# %s\n", o_name);

				/* Dump the object attr/char info */
				auto_dump_printf("K:%d:0x%02X/0x%02X\n\n", i,
					(byte)(k_ptr->x_attr), (byte)(k_ptr->x_char));
			}

			/* Close */
			close_auto_dump();

			/* Message */
			msg_print(_("アイテムの[色/文字]をファイルに書き出しました。", "Dumped object attr/chars."));

			break;
		}

		/* Dump feature attr/chars */
		case '3':
		{
			static cptr mark = "Feature attr/chars";

			/* Prompt */
			prt(_("コマンド: 地形の[色/文字]をファイルに書き出します", "Command: Dump feature attr/chars"), 15, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 17, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Get a filename */
			if (!askfor(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
			auto_dump_printf(_("\n# 地形の[色/文字]の設定\n\n", "\n# Feature attr/char definitions\n\n"));

			/* Dump features */
			for (i = 0; i < max_f_idx; i++)
			{
				feature_type *f_ptr = &f_info[i];

				/* Skip non-entries */
				if (!f_ptr->name) continue;

				/* Skip mimiccing features */
				if (f_ptr->mimic != i) continue;

				/* Dump a comment */
				auto_dump_printf("# %s\n", (f_name + f_ptr->name));

				/* Dump the feature attr/char info */
				auto_dump_printf("F:%d:0x%02X/0x%02X:0x%02X/0x%02X:0x%02X/0x%02X\n\n", i,
					(byte)(f_ptr->x_attr[F_LIT_STANDARD]), (byte)(f_ptr->x_char[F_LIT_STANDARD]),
					(byte)(f_ptr->x_attr[F_LIT_LITE]), (byte)(f_ptr->x_char[F_LIT_LITE]),
					(byte)(f_ptr->x_attr[F_LIT_DARK]), (byte)(f_ptr->x_char[F_LIT_DARK]));
			}

			/* Close */
			close_auto_dump();

			/* Message */
			msg_print(_("地形の[色/文字]をファイルに書き出しました。", "Dumped feature attr/chars."));

			break;
		}

		/* Modify monster attr/chars (numeric operation) */
		case '4':
		{
			static cptr choice_msg = _("モンスターの[色/文字]を変更します", "Change monster attr/chars");
			static int r = 0;

			prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				monster_race *r_ptr = &r_info[r];
				char c;
				int t;

				byte da = r_ptr->d_attr;
				byte dc = r_ptr->d_char;
				byte ca = r_ptr->x_attr;
				byte cc = r_ptr->x_char;

				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					 format(_("モンスター = %d, 名前 = %-40.40s", "Monster = %d, Name = %-40.40s"), r, (r_name + r_ptr->name)));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					format(_("初期値  色 / 文字 = %3u / %3u", "Default attr/char = %3u / %3u"), da, dc));

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 19, da, dc, 0, 0);

				/* Label the Current values */
				Term_putstr(10, 20, -1, TERM_WHITE,
					format(_("現在値  色 / 文字 = %3u / %3u", "Current attr/char = %3u / %3u"), ca, cc));

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 20, ca, cc, 0, 0);

				/* Prompt */
				Term_putstr(0, 22, -1, TERM_WHITE, 
					_("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));

				/* Get a command */
				i = inkey();

				/* All done */
				if (i == ESCAPE) break;

				if (iscntrl(i)) c = 'a' + i - KTRL('A');
				else if (isupper(i)) c = 'a' + i - 'A';
				else c = i;

				switch (c)
				{
				case 'n':
					{
						int prev_r = r;
						do
						{
							if (!cmd_visuals_aux(i, &r, max_r_idx))
							{
								r = prev_r;
								break;
							}
						}
						while (!r_info[r].name);
					}
					break;
				case 'a':
					t = (int)r_ptr->x_attr;
					(void)cmd_visuals_aux(i, &t, 256);
					r_ptr->x_attr = (byte)t;
					need_redraw = TRUE;
					break;
				case 'c':
					t = (int)r_ptr->x_char;
					(void)cmd_visuals_aux(i, &t, 256);
					r_ptr->x_char = (byte)t;
					need_redraw = TRUE;
					break;
				case 'v':
					do_cmd_knowledge_monsters(&need_redraw, TRUE, r);

					/* Clear screen */
					Term_clear();
					print_visuals_menu(choice_msg);
					break;
				}
			}

			break;
		}

		/* Modify object attr/chars (numeric operation) */
		case '5':
		{
			static cptr choice_msg = _("アイテムの[色/文字]を変更します", "Change object attr/chars");
			static int k = 0;
			prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				object_kind *k_ptr = &k_info[k];
				char c;
				int t;

				byte da = k_ptr->d_attr;
				byte dc = k_ptr->d_char;
				byte ca = k_ptr->x_attr;
				byte cc = k_ptr->x_char;

				/* Label the object */
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format(_("アイテム = %d, 名前 = %-40.40s", "Object = %d, Name = %-40.40s"),
						   k, k_name + (!k_ptr->flavor ? k_ptr->name : k_ptr->flavor_name)));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), da, dc));

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 19, da, dc, 0, 0);

				/* Label the Current values */
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format(_("現在値  色 / 文字 = %3d / %3d", "Current attr/char = %3d / %3d"), ca, cc));

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 20, ca, cc, 0, 0);

				/* Prompt */
				Term_putstr(0, 22, -1, TERM_WHITE,
					    _("コマンド (n/N/^N/a/A/^A/c/C/^C/v/V/^V): ", "Command (n/N/^N/a/A/^A/c/C/^C/v/V/^V): "));

				/* Get a command */
				i = inkey();

				/* All done */
				if (i == ESCAPE) break;

				if (iscntrl(i)) c = 'a' + i - KTRL('A');
				else if (isupper(i)) c = 'a' + i - 'A';
				else c = i;

				switch (c)
				{
				case 'n':
					{
						int prev_k = k;
						do
						{
							if (!cmd_visuals_aux(i, &k, max_k_idx))
							{
								k = prev_k;
								break;
							}
						}
						while (!k_info[k].name);
					}
					break;
				case 'a':
					t = (int)k_ptr->x_attr;
					(void)cmd_visuals_aux(i, &t, 256);
					k_ptr->x_attr = (byte)t;
					need_redraw = TRUE;
					break;
				case 'c':
					t = (int)k_ptr->x_char;
					(void)cmd_visuals_aux(i, &t, 256);
					k_ptr->x_char = (byte)t;
					need_redraw = TRUE;
					break;
				case 'v':
					do_cmd_knowledge_objects(&need_redraw, TRUE, k);

					/* Clear screen */
					Term_clear();
					print_visuals_menu(choice_msg);
					break;
				}
			}

			break;
		}

		/* Modify feature attr/chars (numeric operation) */
		case '6':
		{
			static cptr choice_msg = _("地形の[色/文字]を変更します", "Change feature attr/chars");
			static int f = 0;
			static int lighting_level = F_LIT_STANDARD;
			prt(format(_("コマンド: %s", "Command: %s"), choice_msg), 15, 0);

			/* Hack -- query until done */
			while (1)
			{
				feature_type *f_ptr = &f_info[f];
				char c;
				int t;

				byte da = f_ptr->d_attr[lighting_level];
				byte dc = f_ptr->d_char[lighting_level];
				byte ca = f_ptr->x_attr[lighting_level];
				byte cc = f_ptr->x_char[lighting_level];

				/* Label the object */
				prt("", 17, 5);
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format(_("地形 = %d, 名前 = %s, 明度 = %s", "Terrain = %d, Name = %s, Lighting = %s"),
						   f, (f_name + f_ptr->name), lighting_level_str[lighting_level]));

				/* Label the Default values */
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format(_("初期値  色 / 文字 = %3d / %3d", "Default attr/char = %3d / %3d"), da, dc));

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 19, da, dc, 0, 0);

				/* Label the Current values */
#ifdef JP
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("現在値  色 / 文字 = %3d / %3d", ca, cc));
#else
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("Current attr/char = %3d / %3d", ca, cc));
#endif

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
				Term_queue_bigchar(43, 20, ca, cc, 0, 0);

				/* Prompt */
#ifdef JP
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "コマンド (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): ");
#else
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "Command (n/N/^N/a/A/^A/c/C/^C/l/L/^L/d/D/^D/v/V/^V): ");
#endif

				/* Get a command */
				i = inkey();

				/* All done */
				if (i == ESCAPE) break;

				if (iscntrl(i)) c = 'a' + i - KTRL('A');
				else if (isupper(i)) c = 'a' + i - 'A';
				else c = i;

				switch (c)
				{
				case 'n':
					{
						int prev_f = f;
						do
						{
							if (!cmd_visuals_aux(i, &f, max_f_idx))
							{
								f = prev_f;
								break;
							}
						}
						while (!f_info[f].name || (f_info[f].mimic != f));
					}
					break;
				case 'a':
					t = (int)f_ptr->x_attr[lighting_level];
					(void)cmd_visuals_aux(i, &t, 256);
					f_ptr->x_attr[lighting_level] = (byte)t;
					need_redraw = TRUE;
					break;
				case 'c':
					t = (int)f_ptr->x_char[lighting_level];
					(void)cmd_visuals_aux(i, &t, 256);
					f_ptr->x_char[lighting_level] = (byte)t;
					need_redraw = TRUE;
					break;
				case 'l':
					(void)cmd_visuals_aux(i, &lighting_level, F_LIT_MAX);
					break;
				case 'd':
					apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
					need_redraw = TRUE;
					break;
				case 'v':
					do_cmd_knowledge_features(&need_redraw, TRUE, f, &lighting_level);

					/* Clear screen */
					Term_clear();
					print_visuals_menu(choice_msg);
					break;
				}
			}

			break;
		}

		/* Modify monster attr/chars (visual mode) */
		case '7':
			do_cmd_knowledge_monsters(&need_redraw, TRUE, -1);
			break;

		/* Modify object attr/chars (visual mode) */
		case '8':
			do_cmd_knowledge_objects(&need_redraw, TRUE, -1);
			break;

		/* Modify feature attr/chars (visual mode) */
		case '9':
		{
			int lighting_level = F_LIT_STANDARD;
			do_cmd_knowledge_features(&need_redraw, TRUE, -1, &lighting_level);
			break;
		}

#endif /* ALLOW_VISUALS */

		/* Reset visuals */
		case 'R':
		case 'r':
			/* Reset */
			reset_visuals();

			/* Message */
			msg_print(_("画面上の[色/文字]を初期値にリセットしました。", "Visual attr/char tables reset."));
			need_redraw = TRUE;
			break;

		/* Unknown option */
		default:
			bell();
			break;
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Restore the screen */
	screen_load();

	if (need_redraw) do_cmd_redraw();
}


/*
 * Interact with "colors"
 */
void do_cmd_colors(void)
{
	int i;

	char tmp[160];

	char buf[1024];


	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);


	/* Save the screen */
	screen_save();


	/* Interact until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Ask for a choice */
		prt(_("[ カラーの設定 ]", "Interact with Colors"), 2, 0);

		/* Give some choices */
		prt(_("(1) ユーザー設定ファイルのロード", "(1) Load a user pref file"), 4, 5);

#ifdef ALLOW_COLORS
		prt(_("(2) カラーの設定をファイルに書き出す", "(2) Dump colors"), 5, 5);
		prt(_("(3) カラーの設定を変更する", "(3) Modify colors"), 6, 5);
#endif

		/* Prompt */
		prt(_("コマンド: ", "Command: "), 8, 0);
		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;

		/* Load a 'pref' file */
		if (i == '1')
		{
			/* Prompt */
			prt(_("コマンド: ユーザー設定ファイルをロードします", "Command: Load a user pref file"), 8, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 10, 0);

			/* Default file */
			sprintf(tmp, "%s.prf", player_base);

			/* Query */
			if (!askfor(tmp, 70)) continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);

			/* Mega-Hack -- react to changes */
			Term_xtra(TERM_XTRA_REACT, 0);

			/* Mega-Hack -- redraw */
			Term_redraw();
		}

#ifdef ALLOW_COLORS

		/* Dump colors */
		else if (i == '2')
		{
			static cptr mark = "Colors";

			/* Prompt */
			prt(_("コマンド: カラーの設定をファイルに書き出します", "Command: Dump colors"), 8, 0);

			/* Prompt */
			prt(_("ファイル: ", "File: "), 10, 0);

			/* Default filename */
			sprintf(tmp, "%s.prf", player_base);

			/* Get a filename */
			if (!askfor(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
			auto_dump_printf(_("\n# カラーの設定\n\n", "\n# Color redefinitions\n\n"));
			
			/* Dump colors */
			for (i = 0; i < 256; i++)
			{
				int kv = angband_color_table[i][0];
				int rv = angband_color_table[i][1];
				int gv = angband_color_table[i][2];
				int bv = angband_color_table[i][3];

				cptr name = _("未知", "unknown");

				/* Skip non-entries */
				if (!kv && !rv && !gv && !bv) continue;

				/* Extract the color name */
				if (i < 16) name = color_names[i];

				/* Dump a comment */
				auto_dump_printf(_("# カラー '%s'\n", "# Color '%s'\n"), name);
				
				/* Dump the monster attr/char info */
				auto_dump_printf("V:%d:0x%02X:0x%02X:0x%02X:0x%02X\n\n",
					i, kv, rv, gv, bv);
			}

			/* Close */
			close_auto_dump();

			/* Message */
			msg_print(_("カラーの設定をファイルに書き出しました。", "Dumped color redefinitions."));
		}

		/* Edit colors */
		else if (i == '3')
		{
			static byte a = 0;

			/* Prompt */
			prt(_("コマンド: カラーの設定を変更します", "Command: Modify colors"), 8, 0);

			/* Hack -- query until done */
			while (1)
			{
				cptr name;
				byte j;

				/* Clear */
				clear_from(10);

				/* Exhibit the normal colors */
				for (j = 0; j < 16; j++)
				{
					/* Exhibit this color */
					Term_putstr(j*4, 20, -1, a, "###");

					/* Exhibit all colors */
					Term_putstr(j*4, 22, -1, j, format("%3d", j));
				}

				/* Describe the color */
				name = ((a < 16) ? color_names[a] : _("未定義", "undefined"));

				/* Describe the color */
				Term_putstr(5, 10, -1, TERM_WHITE,
					    format(_("カラー = %d, 名前 = %s", "Color = %d, Name = %s"), a, name));

				/* Label the Current values */
				Term_putstr(5, 12, -1, TERM_WHITE,
					    format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x",
						   angband_color_table[a][0],
						   angband_color_table[a][1],
						   angband_color_table[a][2],
						   angband_color_table[a][3]));

				/* Prompt */
				Term_putstr(0, 14, -1, TERM_WHITE,
					_("コマンド (n/N/k/K/r/R/g/G/b/B): ", "Command (n/N/k/K/r/R/g/G/b/B): "));


				/* Get a command */
				i = inkey();

				/* All done */
				if (i == ESCAPE) break;

				/* Analyze */
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

				/* Hack -- react to changes */
				Term_xtra(TERM_XTRA_REACT, 0);

				/* Hack -- redraw */
				Term_redraw();
			}
		}

#endif

		/* Unknown option */
		else
		{
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}


	/* Restore the screen */
	screen_load();
}


/*
 * Note something in the message recall
 */
void do_cmd_note(void)
{
	char buf[80];

	/* Default */
	strcpy(buf, "");

	/* Input */
	if (!get_string(_("メモ: ", "Note: "), buf, 60)) return;

	/* Ignore empty notes */
	if (!buf[0] || (buf[0] == ' ')) return;

	/* Add the note to the message recall */
	msg_format(_("メモ: %s", "Note: %s"), buf);
}


/*
 * Mention the current version
 */
void do_cmd_version(void)
{
	/* Silly message */
	msg_format(_("変愚蛮怒(Hengband) %d.%d.%d", "You are playing Hengband %d.%d.%d."),
				FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
}



/*
 * Array of feeling strings
 */
static cptr do_cmd_feeling_text[11] =
{
	_("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
	_("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
	_("恐ろしい死の幻が目に浮かび、気絶しそうになった！", "You nearly faint as horrible visions of death fill your mind!"),
	_("この階はとても危険なようだ。", "This level looks very dangerous."),
	_("とても悪い予感がする...", "You have a very bad feeling..."),
	_("悪い予感がする...", "You have a bad feeling..."),
	_("何か緊張する。", "You feel nervous."),
	_("少し不運な気がする...", "You feel your luck is turning..."),
	_("この場所は好きになれない。", "You don't like the look of this place."),
	_("この階はそれなりに安全なようだ。", "This level looks reasonably safe."),
	_("なんて退屈なところだ...", "What a boring place...")
};

static cptr do_cmd_feeling_text_combat[11] =
{
	_("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
	_("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
	_("今夜もまた、誰かが命を落とす...", "You nearly faint as horrible visions of death fill your mind!"),
	_("この階はとても危険なようだ。", "This level looks very dangerous."),
	_("とても悪い予感がする...", "You have a very bad feeling..."),
	_("悪い予感がする...", "You have a bad feeling..."),
	_("何か緊張する。", "You feel nervous."),
	_("少し不運な気がする...", "You feel your luck is turning..."),
	_("この場所は好きになれない。", "You don't like the look of this place."),
	_("この階はそれなりに安全なようだ。", "This level looks reasonably safe."),
	_("なんて退屈なところだ...", "What a boring place...")
};

static cptr do_cmd_feeling_text_lucky[11] =
{
	_("この階の雰囲気を感じとれなかった...", "Looks like any other level."),
	_("この階には何か特別なものがあるような気がする。", "You feel there is something special about this level."),
	_("この階はこの上なく素晴らしい感じがする。", "You have a superb feeling about this level."),
	_("素晴らしい感じがする...", "You have an excellent feeling..."),
	_("とても良い感じがする...", "You have a very good feeling..."),
	_("良い感じがする...", "You have a good feeling..."),
	_("ちょっと幸運な感じがする...", "You feel strangely lucky..."),
	_("多少は運が向いてきたか...", "You feel your luck is turning..."),
	_("見た感じ悪くはない...", "You like the look of this place..."),
	_("全然駄目ということはないが...", "This level can't be all bad..."),
	_("なんて退屈なところだ...", "What a boring place...")
};


/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(void)
{
	/* No useful feeling in quests */
	if (p_ptr->inside_quest && !random_quest_number(dun_level))
	{
		msg_print(_("典型的なクエストのダンジョンのようだ。", "Looks like a typical quest level."));
		return;
	}

	/* No useful feeling in town */
	else if (p_ptr->town_num && !dun_level)
	{
		if (!strcmp(town[p_ptr->town_num].name, _("荒野", "wilderness")))
		{
			msg_print(_("何かありそうな荒野のようだ。", "Looks like a strange wilderness."));
			return;
		}
		else
		{
			msg_print(_("典型的な町のようだ。", "Looks like a typical town."));
			return;
		}
	}

	/* No useful feeling in the wilderness */
	else if (!dun_level)
	{
		msg_print(_("典型的な荒野のようだ。", "Looks like a typical wilderness."));
		return;
	}

	/* Display the feeling */
	if (p_ptr->muta3 & MUT3_GOOD_LUCK)
		msg_print(do_cmd_feeling_text_lucky[p_ptr->feeling]);
	else if (p_ptr->pseikaku == SEIKAKU_COMBAT ||
		 inventory[INVEN_BOW].name1 == ART_CRIMSON)
		msg_print(do_cmd_feeling_text_combat[p_ptr->feeling]);
	else
		msg_print(do_cmd_feeling_text[p_ptr->feeling]);
}



/*
 * Description of each monster group.
 */
static cptr monster_group_text[] = 
{
#ifdef JP
	"ユニーク",	/* "Uniques" */
	"乗馬可能なモンスター",	/* "Riding" */
	"賞金首", /* "Wanted */
	"アンバーの王族", /* "Ambertite" */
	"アリ",
	"コウモリ",
	"ムカデ",
	"ドラゴン",
	"目玉",
	"ネコ",
	"ゴーレム",
	"標準人間型生物",
	"ベトベト",
	"ゼリー",
	"コボルド",
	"水棲生物",
	"モルド",
	"ナーガ",
	"オーク",
	"人間",
	"四足獣",
	"ネズミ",
	"スケルトン",
	"デーモン",
	"ボルテックス",
	"イモムシ/大群",
	/* "unused", */
	"イーク",
	"ゾンビ/ミイラ",
	"天使",
	"鳥",
	"犬",
	/* "古代ドラゴン/ワイアーム", */
	"エレメンタル",
	"トンボ",
	"ゴースト",
	"雑種",
	"昆虫",
	"ヘビ",
	"キラー・ビートル",
	"リッチ",
	"多首の爬虫類",
	"謎の生物",
	"オーガ",
	"巨大人間型生物",
	"クイルスルグ",
	"爬虫類/両生類",
	"蜘蛛/サソリ/ダニ",
	"トロル",
	/* "上級デーモン", */
	"バンパイア",
	"ワイト/レイス/等",
	"ゾーン/ザレン/等",
	"イエティ",
	"ハウンド",
	"ミミック",
	"壁/植物/気体",
	"おばけキノコ",
	"球体",
	"プレイヤー",
#else
	"Uniques",
	"Ridable monsters",
	"Wanted monsters",
	"Ambertite",
	"Ant",
	"Bat",
	"Centipede",
	"Dragon",
	"Floating Eye",
	"Feline",
	"Golem",
	"Hobbit/Elf/Dwarf",
	"Icky Thing",
	"Jelly",
	"Kobold",
	"Aquatic monster",
	"Mold",
	"Naga",
	"Orc",
	"Person/Human",
	"Quadruped",
	"Rodent",
	"Skeleton",
	"Demon",
	"Vortex",
	"Worm/Worm-Mass",
	/* "unused", */
	"Yeek",
	"Zombie/Mummy",
	"Angel",
	"Bird",
	"Canine",
	/* "Ancient Dragon/Wyrm", */
	"Elemental",
	"Dragon Fly",
	"Ghost",
	"Hybrid",
	"Insect",
	"Snake",
	"Killer Beetle",
	"Lich",
	"Multi-Headed Reptile",
	"Mystery Living",
	"Ogre",
	"Giant Humanoid",
	"Quylthulg",
	"Reptile/Amphibian",
	"Spider/Scorpion/Tick",
	"Troll",
	/* "Major Demon", */
	"Vampire",
	"Wight/Wraith/etc",
	"Xorn/Xaren/etc",
	"Yeti",
	"Zephyr Hound",
	"Mimic",
	"Wall/Plant/Gas",
	"Mushroom patch",
	"Ball",
	"Player",
#endif
	NULL
};


/*
 * Symbols of monsters in each group. Note the "Uniques" group
 * is handled differently.
 */
static cptr monster_group_char[] =
{
	(char *) -1L,
	(char *) -2L,
	(char *) -3L,
	(char *) -4L,
	"a",
	"b",
	"c",
	"dD",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"pt",
	"q",
	"r",
	"s",
	"uU",
	"v",
	"w",
	/* "x", */
	"y",
	"z",
	"A",
	"B",
	"C",
	/* "D", */
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	/* "U", */
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"!$&()+./=>?[\\]`{|~",
	"#%",
	",",
	"*",
	"@",
	NULL
};


/*
 * hook function to sort monsters by level
 */
static bool ang_sort_comp_monster_level(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	int w1 = who[a];
	int w2 = who[b];

	monster_race *r_ptr1 = &r_info[w1];
	monster_race *r_ptr2 = &r_info[w2];

	/* Unused */
	(void)v;

	if (r_ptr2->level > r_ptr1->level) return TRUE;
	if (r_ptr1->level > r_ptr2->level) return FALSE;

	if ((r_ptr2->flags1 & RF1_UNIQUE) && !(r_ptr1->flags1 & RF1_UNIQUE)) return TRUE;
	if ((r_ptr1->flags1 & RF1_UNIQUE) && !(r_ptr2->flags1 & RF1_UNIQUE)) return FALSE;
	return w1 <= w2;
}

/*
 * Build a list of monster indexes in the given group. Return the number
 * of monsters in the group.
 *
 * mode & 0x01 : check for non-empty group
 * mode & 0x02 : visual operation only
 */
static int collect_monsters(int grp_cur, s16b mon_idx[], byte mode)
{
	int i, mon_cnt = 0;
	int dummy_why;

	/* Get a list of x_char in this group */
	cptr group_char = monster_group_char[grp_cur];

	/* XXX Hack -- Check if this is the "Uniques" group */
	bool grp_unique = (monster_group_char[grp_cur] == (char *) -1L);

	/* XXX Hack -- Check if this is the "Riding" group */
	bool grp_riding = (monster_group_char[grp_cur] == (char *) -2L);

	/* XXX Hack -- Check if this is the "Wanted" group */
	bool grp_wanted = (monster_group_char[grp_cur] == (char *) -3L);

	/* XXX Hack -- Check if this is the "Amberite" group */
	bool grp_amberite = (monster_group_char[grp_cur] == (char *) -4L);


	/* Check every race */
	for (i = 0; i < max_r_idx; i++)
	{
		/* Access the race */
		monster_race *r_ptr = &r_info[i];

		/* Skip empty race */
		if (!r_ptr->name) continue ;

		/* Require known monsters */
		if (!(mode & 0x02) && !cheat_know && !r_ptr->r_sights) continue;

		if (grp_unique)
		{
			if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
		}

		else if (grp_riding)
		{
			if (!(r_ptr->flags7 & RF7_RIDING)) continue;
		}

		else if (grp_wanted)
		{
			bool wanted = FALSE;
			int j;
			for (j = 0; j < MAX_KUBI; j++)
			{
				if (kubi_r_idx[j] == i || kubi_r_idx[j] - 10000 == i ||
					(p_ptr->today_mon && p_ptr->today_mon == i))
				{
					wanted = TRUE;
					break;
				}
			}
			if (!wanted) continue;
		}

		else if (grp_amberite)
		{
			if (!(r_ptr->flags3 & RF3_AMBERITE)) continue;
		}

		else
		{
			/* Check for race in the group */
			if (!my_strchr(group_char, r_ptr->d_char)) continue;
		}

		/* Add the race */
		mon_idx[mon_cnt++] = i;

		/* XXX Hack -- Just checking for non-empty group */
		if (mode & 0x01) break;
	}

	/* Terminate the list */
	mon_idx[mon_cnt] = -1;

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_monster_level;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort by monster level */
	ang_sort(mon_idx, &dummy_why, mon_cnt);

	/* Return the number of races */
	return mon_cnt;
}


/*
 * Description of each monster group.
 */
static cptr object_group_text[] = 
{
#ifdef JP
	"キノコ",	/* "Mushrooms" */
	"薬",		/* "Potions" */
	"油つぼ",	/* "Flasks" */
	"巻物",		/* "Scrolls" */
	"指輪",		/* "Rings" */
	"アミュレット",	/* "Amulets" */
	"笛",		/* "Whistle" */
	"光源",		/* "Lanterns" */
	"魔法棒",	/* "Wands" */
	"杖",		/* "Staffs" */
	"ロッド",	/* "Rods" */
	"カード",	/* "Cards" */
	"キャプチャー・ボール",
	"羊皮紙",	
	"くさび",
	"箱",
	"人形",
	"像",
	"ゴミ",
	"空のビン",
	"骨",
	"死体",
	"刀剣類",	/* "Swords" */
	"鈍器",		/* "Blunt Weapons" */
	"長柄武器",	/* "Polearms" */
	"採掘道具",	/* "Diggers" */
	"飛び道具",	/* "Bows" */
	"弾",
	"矢",
	"ボルト",
	"軽装鎧",	/* "Soft Armor" */
	"重装鎧",	/* "Hard Armor" */
	"ドラゴン鎧",	/* "Dragon Armor" */
	"盾",	/* "Shields" */
	"クローク",	/* "Cloaks" */
	"籠手",	/* "Gloves" */
	"ヘルメット",	/* "Helms" */
	"冠",	/* "Crowns" */
	"ブーツ",	/* "Boots" */
	"魔法書",
	"財宝",
	"何か",
#else
	"Mushrooms",
	"Potions",
	"Flasks",
	"Scrolls",
	"Rings",
	"Amulets",
	"Whistle",
	"Lanterns",
	"Wands",
	"Staves",
	"Rods",
	"Cards",
	"Capture Balls",
	"Parchments",
	"Spikes",
	"Boxs",
	"Figurines",
	"Statues",
	"Junks",
	"Bottles",
	"Skeletons",
	"Corpses",
	"Swords",
	"Blunt Weapons",
	"Polearms",
	"Diggers",
	"Bows",
	"Shots",
	"Arrows",
	"Bolts",
	"Soft Armor",
	"Hard Armor",
	"Dragon Armor",
	"Shields",
	"Cloaks",
	"Gloves",
	"Helms",
	"Crowns",
	"Boots",
	"Spellbooks",
	"Treasure",
	"Something",
#endif
	NULL
};


/*
 * TVALs of items in each group
 */
static byte object_group_tval[] = 
{
	TV_FOOD,
	TV_POTION,
	TV_FLASK,
	TV_SCROLL,
	TV_RING,
	TV_AMULET,
	TV_WHISTLE,
	TV_LITE,
	TV_WAND,
	TV_STAFF,
	TV_ROD,
	TV_CARD,
	TV_CAPTURE,
	TV_PARCHMENT,
	TV_SPIKE,
	TV_CHEST,
	TV_FIGURINE,
	TV_STATUE,
	TV_JUNK,
	TV_BOTTLE,
	TV_SKELETON,
	TV_CORPSE,
	TV_SWORD,
	TV_HAFTED,
	TV_POLEARM,
	TV_DIGGING,
	TV_BOW,
	TV_SHOT,
	TV_ARROW,
	TV_BOLT,
	TV_SOFT_ARMOR,
	TV_HARD_ARMOR,
	TV_DRAG_ARMOR,
	TV_SHIELD,
	TV_CLOAK,
	TV_GLOVES,
	TV_HELM,
	TV_CROWN,
	TV_BOOTS,
	TV_LIFE_BOOK, /* Hack -- all spellbooks */
	TV_GOLD,
	0,
	0,
};


/*
 * Build a list of object indexes in the given group. Return the number
 * of objects in the group.
 *
 * mode & 0x01 : check for non-empty group
 * mode & 0x02 : visual operation only
 */
static int collect_objects(int grp_cur, int object_idx[], byte mode)
{
	int i, j, k, object_cnt = 0;

	/* Get a list of x_char in this group */
	byte group_tval = object_group_tval[grp_cur];

	/* Check every object */
	for (i = 0; i < max_k_idx; i++)
	{
		/* Access the object */
		object_kind *k_ptr = &k_info[i];

		/* Skip empty objects */
		if (!k_ptr->name) continue;

		if (mode & 0x02)
		{
			/* Any objects will be displayed */
		}
		else
		{
			if (!p_ptr->wizard)
			{
				/* Skip non-flavoured objects */
				if (!k_ptr->flavor) continue;

				/* Require objects ever seen */
				if (!k_ptr->aware) continue;
			}

			/* Skip items with no distribution (special artifacts) */
			for (j = 0, k = 0; j < 4; j++) k += k_ptr->chance[j];
			if (!k) continue;
		}

		/* Check for objects in the group */
		if (TV_LIFE_BOOK == group_tval)
		{
			/* Hack -- All spell books */
			if (TV_LIFE_BOOK <= k_ptr->tval && k_ptr->tval <= TV_HEX_BOOK)
			{
				/* Add the object */
				object_idx[object_cnt++] = i;
			}
			else continue;
		}
		else if (k_ptr->tval == group_tval)
		{
			/* Add the object */
			object_idx[object_cnt++] = i;
		}
		else continue;

		/* XXX Hack -- Just checking for non-empty group */
		if (mode & 0x01) break;
	}

	/* Terminate the list */
	object_idx[object_cnt] = -1;

	/* Return the number of objects */
	return object_cnt;
}


/*
 * Description of each feature group.
 */
static cptr feature_group_text[] = 
{
	"terrains",
	NULL
};


/*
 * Build a list of feature indexes in the given group. Return the number
 * of features in the group.
 *
 * mode & 0x01 : check for non-empty group
 */
static int collect_features(int grp_cur, int *feat_idx, byte mode)
{
	int i, feat_cnt = 0;

	/* Unused;  There is a single group. */
	(void)grp_cur;

	/* Check every feature */
	for (i = 0; i < max_f_idx; i++)
	{
		/* Access the index */
		feature_type *f_ptr = &f_info[i];

		/* Skip empty index */
		if (!f_ptr->name) continue;

		/* Skip mimiccing features */
		if (f_ptr->mimic != i) continue;

		/* Add the index */
		feat_idx[feat_cnt++] = i;

		/* XXX Hack -- Just checking for non-empty group */
		if (mode & 0x01) break;
	}

	/* Terminate the list */
	feat_idx[feat_cnt] = -1;

	/* Return the number of races */
	return feat_cnt;
}


#if 0
/*
 * Build a list of monster indexes in the given group. Return the number
 * of monsters in the group.
 */
static int collect_artifacts(int grp_cur, int object_idx[])
{
	int i, object_cnt = 0;

	/* Get a list of x_char in this group */
	byte group_tval = object_group_tval[grp_cur];

	/* Check every object */
	for (i = 0; i < max_a_idx; i++)
	{
		/* Access the artifact */
		artifact_type *a_ptr = &a_info[i];

		/* Skip empty artifacts */
		if (!a_ptr->name) continue;

		/* Skip "uncreated" artifacts */
		if (!a_ptr->cur_num) continue;

		/* Check for race in the group */
		if (a_ptr->tval == group_tval)
		{
			/* Add the race */
			object_idx[object_cnt++] = i;
		}
	}

	/* Terminate the list */
	object_idx[object_cnt] = 0;

	/* Return the number of races */
	return object_cnt;
}
#endif /* 0 */


/*
 * Encode the screen colors
 */
static char hack[17] = "dwsorgbuDWvyRGBU";


/*
 * Hack -- load a screen dump from a file
 */
void do_cmd_load_screen(void)
{
	int i, y, x;

	byte a = 0;
	char c = ' ';

	bool okay = TRUE;

	FILE *fff;

	char buf[1024];

	int wid, hgt;

	Term_get_size(&wid, &hgt);

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");

	/* Append to the file */
	fff = my_fopen(buf, "r");

	/* Oops */
	if (!fff) {
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), buf);
		msg_print(NULL);
		return;
	}


	/* Save the screen */
	screen_save();

	/* Clear the screen */
	Term_clear();


	/* Load the screen */
	for (y = 0; okay; y++)
	{
		/* Get a line of data including control code */
		if (!fgets(buf, 1024, fff)) okay = FALSE;

		/* Get the blank line */
		if (buf[0] == '\n' || buf[0] == '\0') break;

		/* Ignore too large screen image */
		if (y >= hgt) continue;

		/* Show each row */
		for (x = 0; x < wid - 1; x++)
		{
			/* End of line */
			if (buf[x] == '\n' || buf[x] == '\0') break;

			/* Put the attr/char */
			Term_draw(x, y, TERM_WHITE, buf[x]);
		}
	}

	/* Dump the screen */
	for (y = 0; okay; y++)
	{
		/* Get a line of data including control code */
		if (!fgets(buf, 1024, fff)) okay = FALSE;

		/* Get the blank line */
		if (buf[0] == '\n' || buf[0] == '\0') break;

		/* Ignore too large screen image */
		if (y >= hgt) continue;

		/* Dump each row */
		for (x = 0; x < wid - 1; x++)
		{
			/* End of line */
			if (buf[x] == '\n' || buf[x] == '\0') break;

			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Look up the attr */
			for (i = 0; i < 16; i++)
			{
				/* Use attr matches */
				if (hack[i] == buf[x]) a = i;
			}

			/* Put the attr/char */
			Term_draw(x, y, a, c);
		}
	}


	/* Close it */
	my_fclose(fff);


	/* Message */
	prt(_("ファイルに書き出された画面(記念撮影)をロードしました。", "Screen dump loaded."), 0, 0);

	flush();
	inkey();


	/* Restore the screen */
	screen_load();
}




cptr inven_res_label = _("                               酸電火冷毒光闇破轟獄因沌劣 盲怖乱痺透命感消復浮",
						 "                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSeHlEpSdRgLv");


#define IM_FLAG_STR  _("＊", "* ")
#define HAS_FLAG_STR _("＋", "+ ")
#define NO_FLAG_STR  _("・", ". ")

#define print_im_or_res_flag(IM, RES) \
{ \
	fputs(have_flag(flgs, (IM)) ? IM_FLAG_STR : \
	      (have_flag(flgs, (RES)) ? HAS_FLAG_STR : NO_FLAG_STR), fff); \
}

#define print_flag(TR) \
{ \
	fputs(have_flag(flgs, (TR)) ? HAS_FLAG_STR : NO_FLAG_STR, fff); \
}


/* XTRA HACK RESLIST */
static void do_cmd_knowledge_inven_aux(FILE *fff, object_type *o_ptr, int *j, byte tval, char *where)
{
	char o_name[MAX_NLEN];
	u32b flgs[TR_FLAG_SIZE];

	if (!o_ptr->k_idx) return;
	if (o_ptr->tval != tval) return;

	/* Identified items only */
	if (!object_is_known(o_ptr)) return;

	/*
	 * HACK:Ring of Lordly protection and Dragon equipment
	 * have random resistances.
	 */
	if ((object_is_wearable(o_ptr) && object_is_ego(o_ptr))
	    || ((tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_RESISTANCE))
	    || ((tval == TV_RING) && (o_ptr->sval == SV_RING_LORDLY))
	    || ((tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD))
	    || ((tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM))
	    || ((tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES))
	    || ((tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE))
	    || object_is_artifact(o_ptr))
	{
		int i = 0;
		object_desc(o_name, o_ptr, OD_NAME_ONLY);

		while (o_name[i] && (i < 26))
		{
#ifdef JP
			if (iskanji(o_name[i])) i++;
#endif
			i++;
		}

		if (i < 28)
		{
			while (i < 28)
			{
				o_name[i] = ' '; i++;
			}
		}
		o_name[i] = '\0';

		fprintf(fff, "%s %s", where, o_name);

		if (!(o_ptr->ident & (IDENT_MENTAL)))
		{
			fputs(_("-------不明--------------- -------不明---------\n", 
					"-------unknown------------ -------unknown------\n"), fff);
		}
		else
		{
			object_flags_known(o_ptr, flgs);

			print_im_or_res_flag(TR_IM_ACID, TR_RES_ACID);
			print_im_or_res_flag(TR_IM_ELEC, TR_RES_ELEC);
			print_im_or_res_flag(TR_IM_FIRE, TR_RES_FIRE);
			print_im_or_res_flag(TR_IM_COLD, TR_RES_COLD);
			print_flag(TR_RES_POIS);
			print_flag(TR_RES_LITE);
			print_flag(TR_RES_DARK);
			print_flag(TR_RES_SHARDS);
			print_flag(TR_RES_SOUND);
			print_flag(TR_RES_NETHER);
			print_flag(TR_RES_NEXUS);
			print_flag(TR_RES_CHAOS);
			print_flag(TR_RES_DISEN);

			fputs(" ", fff);

			print_flag(TR_RES_BLIND);
			print_flag(TR_RES_FEAR);
			print_flag(TR_RES_CONF);
			print_flag(TR_FREE_ACT);
			print_flag(TR_SEE_INVIS);
			print_flag(TR_HOLD_EXP);
			print_flag(TR_TELEPATHY);
			print_flag(TR_SLOW_DIGEST);
			print_flag(TR_REGEN);
			print_flag(TR_LEVITATION);

			fputc('\n', fff);
		}
		(*j)++;
		if (*j == 9)
		{
			*j = 0;
			fprintf(fff, "%s\n", inven_res_label);
		}
	}
}

/*
 * Display *ID* ed weapons/armors's resistances
 */
static void do_cmd_knowledge_inven(void)
{
	FILE *fff;

	char file_name[1024];

	store_type  *st_ptr;

	byte tval;
	int i = 0;
	int j = 0;

	char  where[32];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff)
	{
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}
	fprintf(fff, "%s\n", inven_res_label);

	for (tval = TV_WEARABLE_BEGIN; tval <= TV_WEARABLE_END; tval++)
	{
		if (j != 0)
		{
			for (; j < 9; j++) fputc('\n', fff);
			j = 0;
			fprintf(fff, "%s\n", inven_res_label);
		}
		strcpy(where, _("装", "E "));
		for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			do_cmd_knowledge_inven_aux(fff, &inventory[i], &j, tval, where);
		}
		strcpy(where, _("持", "I "));
		for (i = 0; i < INVEN_PACK; i++)
		{
			do_cmd_knowledge_inven_aux(fff, &inventory[i], &j, tval, where);
		}

		st_ptr = &town[1].store[STORE_HOME];
		strcpy(where, _("家", "H "));
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			do_cmd_knowledge_inven_aux(fff, &st_ptr->stock[i], &j, tval, where);
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("*鑑定*済み武器/防具の耐性リスト", "Resistances of *identified* equipment"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


void do_cmd_save_screen_html_aux(char *filename, int message)
{
	int y, x, i;

	byte a = 0, old_a = 0;
	char c = ' ';

	FILE *fff, *tmpfff;
	char buf[2048];

	int yomikomu = 0;
	cptr tags[4] = {
		"HEADER_START:",
		"HEADER_END:",
		"FOOTER_START:",
		"FOOTER_END:",
	};

	cptr html_head[] = {
		"<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
		"<pre>",
		0,
	};
	cptr html_foot[] = {
		"</pre>\n",
		"</body>\n</html>\n",
		0,
	};

	int wid, hgt;

	Term_get_size(&wid, &hgt);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Append to the file */
	fff = my_fopen(filename, "w");

	/* Oops */
	if (!fff) {
		if (message) {
		    msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename);
		    msg_print(NULL);
		}
		
		return;
	}

	/* Save the screen */
	if (message)
		screen_save();

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "htmldump.prf");
	tmpfff = my_fopen(buf, "r");
	if (!tmpfff) {
		for (i = 0; html_head[i]; i++)
			fputs(html_head[i], fff);
	}
	else {
		yomikomu = 0;
		while (!my_fgets(tmpfff, buf, sizeof(buf))) {
			if (!yomikomu) {
				if (strncmp(buf, tags[0], strlen(tags[0])) == 0)
					yomikomu = 1;
			}
			else {
				if (strncmp(buf, tags[1], strlen(tags[1])) == 0)
					break;
				fprintf(fff, "%s\n", buf);
			}
		}
	}

	/* Dump the screen */
	for (y = 0; y < hgt; y++)
	{
		/* Start the row */
		if (y != 0)
			fprintf(fff, "\n");

		/* Dump each row */
		for (x = 0; x < wid - 1; x++)
		{
			int rv, gv, bv;
			cptr cc = NULL;
			/* Get the attr/char */
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
			if ((y == 0 && x == 0) || a != old_a) {
				rv = angband_color_table[a][1];
				gv = angband_color_table[a][2];
				bv = angband_color_table[a][3];
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

	if (!tmpfff) {
		for (i = 0; html_foot[i]; i++)
			fputs(html_foot[i], fff);
	}
	else {
		rewind(tmpfff);
		yomikomu = 0;
		while (!my_fgets(tmpfff, buf, sizeof(buf))) {
			if (!yomikomu) {
				if (strncmp(buf, tags[2], strlen(tags[2])) == 0)
					yomikomu = 1;
			}
			else {
				if (strncmp(buf, tags[3], strlen(tags[3])) == 0)
					break;
				fprintf(fff, "%s\n", buf);
			}
		}
		my_fclose(tmpfff);
	}

	/* Skip a line */
	fprintf(fff, "\n");

	/* Close it */
	my_fclose(fff);

	/* Message */
	if (message) {
		msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
		msg_print(NULL);
	}

	/* Restore the screen */
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

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

	msg_print(NULL);

	do_cmd_save_screen_html_aux(buf, 1);
}


/*
 * Redefinable "save_screen" action
 */
void (*screendump_aux)(void) = NULL;


/*
 * Hack -- save a screen dump to a file
 */
void do_cmd_save_screen(void)
{
	bool old_use_graphics = use_graphics;
	bool html_dump = FALSE;

	int wid, hgt;

	prt(_("記念撮影しますか？ [(y)es/(h)tml/(n)o] ", "Save screen dump? [(y)es/(h)tml/(n)o] "), 0, 0);
	while(TRUE)
	{
		char c = inkey();
		if (c == 'Y' || c == 'y')
			break;
		else if (c == 'H' || c == 'h')
		{
			html_dump = TRUE;
			break;
		}
		else
		{
			prt("", 0, 0);
			return;
		}
	}

	Term_get_size(&wid, &hgt);

	if (old_use_graphics)
	{
		use_graphics = FALSE;
		reset_visuals();

		/* Redraw everything */
		p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

		/* Hack -- update */
		handle_stuff();
	}

	if (html_dump)
	{
		do_cmd_save_screen_html();
		do_cmd_redraw();
	}

	/* Do we use a special screendump function ? */
	else if (screendump_aux)
	{
		/* Dump the screen to a graphics file */
		(*screendump_aux)();
	}
	else /* Dump the screen as text */
	{
		int y, x;

		byte a = 0;
		char c = ' ';

		FILE *fff;

		char buf[1024];

		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "dump.txt");

		/* File type is "TEXT" */
		FILE_TYPE(FILE_TYPE_TEXT);

		/* Append to the file */
		fff = my_fopen(buf, "w");

		/* Oops */
		if (!fff)
		{
			msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), buf);
			msg_print(NULL);
			return;
		}


		/* Save the screen */
		screen_save();


		/* Dump the screen */
		for (y = 0; y < hgt; y++)
		{
			/* Dump each row */
			for (x = 0; x < wid - 1; x++)
			{
				/* Get the attr/char */
				(void)(Term_what(x, y, &a, &c));

				/* Dump it */
				buf[x] = c;
			}

			/* Terminate */
			buf[x] = '\0';

			/* End the row */
			fprintf(fff, "%s\n", buf);
		}

		/* Skip a line */
		fprintf(fff, "\n");


		/* Dump the screen */
		for (y = 0; y < hgt; y++)
		{
			/* Dump each row */
			for (x = 0; x < wid - 1; x++)
			{
				/* Get the attr/char */
				(void)(Term_what(x, y, &a, &c));

				/* Dump it */
				buf[x] = hack[a&0x0F];
			}

			/* Terminate */
			buf[x] = '\0';

			/* End the row */
			fprintf(fff, "%s\n", buf);
		}

		/* Skip a line */
		fprintf(fff, "\n");


		/* Close it */
		my_fclose(fff);

		/* Message */
		msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
		msg_print(NULL);

		/* Restore the screen */
		screen_load();
	}

	if (old_use_graphics)
	{
		use_graphics = TRUE;
		reset_visuals();

		/* Redraw everything */
		p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

		/* Hack -- update */
		handle_stuff();
	}
}


/*
 * Sorting hook -- Comp function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform on "u".
 */
static bool ang_sort_art_comp(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b *why = (u16b*)(v);

	int w1 = who[a];
	int w2 = who[b];

	int z1, z2;

	/* Sort by total kills */
	if (*why >= 3)
	{
		/* Extract total kills */
		z1 = a_info[w1].tval;
		z2 = a_info[w2].tval;

		/* Compare total kills */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster level */
	if (*why >= 2)
	{
		/* Extract levels */
		z1 = a_info[w1].sval;
		z2 = a_info[w2].sval;

		/* Compare levels */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Sort by monster experience */
	if (*why >= 1)
	{
		/* Extract experience */
		z1 = a_info[w1].level;
		z2 = a_info[w2].level;

		/* Compare experience */
		if (z1 < z2) return (TRUE);
		if (z1 > z2) return (FALSE);
	}


	/* Compare indexes */
	return (w1 <= w2);
}


/*
 * Sorting hook -- Swap function -- see below
 *
 * We use "u" to point to array of monster indexes,
 * and "v" to select the type of sorting to perform.
 */
static void ang_sort_art_swap(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	u16b holder;

	/* Unused */
	(void)v;

	/* Swap */
	holder = who[a];
	who[a] = who[b];
	who[b] = holder;
}


/*
 * Check the status of "artifacts"
 */
static void do_cmd_knowledge_artifacts(void)
{
	int i, k, z, x, y, n = 0;
	u16b why = 3;
	s16b *who;

	FILE *fff;

	char file_name[1024];

	char base_name[MAX_NLEN];

	bool *okay;

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	/* Allocate the "who" array */
	C_MAKE(who, max_a_idx, s16b);

	/* Allocate the "okay" array */
	C_MAKE(okay, max_a_idx, bool);

	/* Scan the artifacts */
	for (k = 0; k < max_a_idx; k++)
	{
		artifact_type *a_ptr = &a_info[k];

		/* Default */
		okay[k] = FALSE;

		/* Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/* Skip "uncreated" artifacts */
		if (!a_ptr->cur_num) continue;

		/* Assume okay */
		okay[k] = TRUE;
	}

	/* Check the dungeon */
	for (y = 0; y < cur_hgt; y++)
	{
		for (x = 0; x < cur_wid; x++)
		{
			cave_type *c_ptr = &cave[y][x];

			s16b this_o_idx, next_o_idx = 0;

			/* Scan all objects in the grid */
			for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;

				/* Acquire object */
				o_ptr = &o_list[this_o_idx];

				/* Acquire next object */
				next_o_idx = o_ptr->next_o_idx;

				/* Ignore non-artifacts */
				if (!object_is_fixed_artifact(o_ptr)) continue;

				/* Ignore known items */
				if (object_is_known(o_ptr)) continue;

				/* Note the artifact */
				okay[o_ptr->name1] = FALSE;
			}
		}
	}

	/* Check the inventory and equipment */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Ignore non-objects */
		if (!o_ptr->k_idx) continue;

		/* Ignore non-artifacts */
		if (!object_is_fixed_artifact(o_ptr)) continue;

		/* Ignore known items */
		if (object_is_known(o_ptr)) continue;

		/* Note the artifact */
		okay[o_ptr->name1] = FALSE;
	}

	for (k = 0; k < max_a_idx; k++)
	{
		if (okay[k]) who[n++] = k;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_art_comp;
	ang_sort_swap = ang_sort_art_swap;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);

	/* Scan the artifacts */
	for (k = 0; k < n; k++)
	{
		artifact_type *a_ptr = &a_info[who[k]];

		/* Paranoia */
		strcpy(base_name, _("未知の伝説のアイテム", "Unknown Artifact"));

		/* Obtain the base object type */
		z = lookup_kind(a_ptr->tval, a_ptr->sval);

		/* Real object */
		if (z)
		{
			object_type forge;
			object_type *q_ptr;

			/* Get local object */
			q_ptr = &forge;

			/* Create fake object */
			object_prep(q_ptr, z);

			/* Make it an artifact */
			q_ptr->name1 = (byte)who[k];

			/* Display as if known */
			q_ptr->ident |= IDENT_STORE;

			/* Describe the artifact */
			object_desc(base_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		}

		/* Hack -- Build the artifact name */
		fprintf(fff, _("     %s\n", "     The %s\n"), base_name);
	}

	/* Free the "who" array */
	C_KILL(who, max_a_idx, s16b);

	/* Free the "okay" array */
	C_KILL(okay, max_a_idx, bool);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("既知の伝説のアイテム", "Artifacts Seen"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display known uniques
 * With "XTRA HACK UNIQHIST" (Originally from XAngband)
 */
static void do_cmd_knowledge_uniques(void)
{
	int i, k, n = 0;
	u16b why = 2;
	s16b *who;

	FILE *fff;

	char file_name[1024];

	int n_alive[10];
	int n_alive_surface = 0;
	int n_alive_over100 = 0;
	int n_alive_total = 0;
	int max_lev = -1;

	for (i = 0; i < 10; i++) n_alive[i] = 0;

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	if (!fff)
	{
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

	/* Scan the monsters */
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];
		int          lev;

		if (!r_ptr->name) continue;

		/* Require unique monsters */
		if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;

		/* Only display "known" uniques */
		if (!cheat_know && !r_ptr->r_sights) continue;

		/* Only print rarity <= 100 uniques */
		if (!r_ptr->rarity || ((r_ptr->rarity > 100) && !(r_ptr->flags1 & RF1_QUESTOR))) continue;

		/* Only "alive" uniques */
		if (r_ptr->max_num == 0) continue;

		if (r_ptr->level)
		{
			lev = (r_ptr->level - 1) / 10;
			if (lev < 10)
			{
				n_alive[lev]++;
				if (max_lev < lev) max_lev = lev;
			}
			else n_alive_over100++;
		}
		else n_alive_surface++;

		/* Collect "appropriate" monsters */
		who[n++] = i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);

	if (n_alive_surface)
	{
		fprintf(fff, _("     地上  生存: %3d体\n", "      Surface  alive: %3d\n"), n_alive_surface);
		n_alive_total += n_alive_surface;
	}
	for (i = 0; i <= max_lev; i++)
	{
		fprintf(fff, _("%3d-%3d階  生存: %3d体\n", "Level %3d-%3d  alive: %3d\n"), 1 + i * 10, 10 + i * 10, n_alive[i]);
		n_alive_total += n_alive[i];
	}
	if (n_alive_over100)
	{
		fprintf(fff, _("101-   階  生存: %3d体\n", "Level 101-     alive: %3d\n"), n_alive_over100);
		n_alive_total += n_alive_over100;
	}

	if (n_alive_total)
	{
		fputs(_("---------  -----------\n", "-------------  ----------\n"), fff);
		fprintf(fff, _("     合計  生存: %3d体\n\n", "        Total  alive: %3d\n\n"), n_alive_total);
	}
	else
	{
		fputs(_("現在は既知の生存ユニークはいません。\n", "No known uniques alive.\n"), fff);
	}

	/* Scan the monster races */
	for (k = 0; k < n; k++)
	{
		monster_race *r_ptr = &r_info[who[k]];

		/* Print a message */
		fprintf(fff, _("     %s (レベル%d)\n", "     %s (level %d)\n"), r_name + r_ptr->name, r_ptr->level);
	}

	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("まだ生きているユニーク・モンスター", "Alive Uniques"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display weapon-exp
 */
static void do_cmd_knowledge_weapon_exp(void)
{
	int i, j, num, weapon_exp;

	FILE *fff;

	char file_name[1024];
	char tmp[30];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	for (i = 0; i < 5; i++)
	{
		for (num = 0; num < 64; num++)
		{
			for (j = 0; j < max_k_idx; j++)
			{
				object_kind *k_ptr = &k_info[j];

				if ((k_ptr->tval == TV_SWORD - i) && (k_ptr->sval == num))
				{
					if ((k_ptr->tval == TV_BOW) && (k_ptr->sval == SV_CRIMSON)) continue;

					weapon_exp = p_ptr->weapon_exp[4 - i][num];
					strip_name(tmp, j);
					fprintf(fff, "%-25s ", tmp);
					if (weapon_exp >= s_info[p_ptr->pclass].w_max[4 - i][num]) fprintf(fff, "!");
					else fprintf(fff, " ");
					fprintf(fff, "%s", exp_level_str[weapon_exp_level(weapon_exp)]);
					if (cheat_xtra) fprintf(fff, " %d", weapon_exp);
					fprintf(fff, "\n");
					break;
				}
			}
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("武器の経験値", "Weapon Proficiency"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*!
 * @brief 魔法の経験値を表示するコマンドのメインルーチン
 * Display spell-exp
 * @return なし
 */
static void do_cmd_knowledge_spell_exp(void)
{
	int i = 0, spell_exp, exp_level;

	FILE *fff;
	const magic_type *s_ptr;

	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	if (p_ptr->realm1 != REALM_NONE)
	{
		fprintf(fff, _("%sの魔法書\n", "%s Spellbook\n"), realm_names[p_ptr->realm1]);
		for (i = 0; i < 32; i++)
		{
			if (!is_magic(p_ptr->realm1))
			{
				s_ptr = &technic_info[p_ptr->realm1 - MIN_TECHNIC][i];
			}
			else
			{
				s_ptr = &mp_ptr->info[p_ptr->realm1 - 1][i];
			}
			if (s_ptr->slevel >= 99) continue;
			spell_exp = p_ptr->spell_exp[i];
			exp_level = spell_exp_level(spell_exp);
			fprintf(fff, "%-25s ", do_spell(p_ptr->realm1, i, SPELL_NAME));
			if (p_ptr->realm1 == REALM_HISSATSU)
				fprintf(fff, "[--]");
			else
			{
				if (exp_level >= EXP_LEVEL_MASTER) fprintf(fff, "!");
				else fprintf(fff, " ");
				fprintf(fff, "%s", exp_level_str[exp_level]);
			}
			if (cheat_xtra) fprintf(fff, " %d", spell_exp);
			fprintf(fff, "\n");
		}
	}

	if (p_ptr->realm2 != REALM_NONE)
	{
		fprintf(fff, _("%sの魔法書\n", "\n%s Spellbook\n"), realm_names[p_ptr->realm2]);
		for (i = 0; i < 32; i++)
		{
			if (!is_magic(p_ptr->realm1))
			{
				s_ptr = &technic_info[p_ptr->realm2 - MIN_TECHNIC][i];
			}
			else
			{
				s_ptr = &mp_ptr->info[p_ptr->realm2 - 1][i];
			}
			if (s_ptr->slevel >= 99) continue;

			spell_exp = p_ptr->spell_exp[i + 32];
			exp_level = spell_exp_level(spell_exp);
			fprintf(fff, "%-25s ", do_spell(p_ptr->realm2, i, SPELL_NAME));
			if (exp_level >= EXP_LEVEL_EXPERT) fprintf(fff, "!");
			else fprintf(fff, " ");
			fprintf(fff, "%s", exp_level_str[exp_level]);
			if (cheat_xtra) fprintf(fff, " %d", spell_exp);
			fprintf(fff, "\n");
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("魔法の経験値", "Spell Proficiency"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*!
 * @brief スキル情報を表示するコマンドのメインルーチン /
 * Display skill-exp
 * @return なし
 */
static void do_cmd_knowledge_skill_exp(void)
{
	int i = 0, skill_exp;

	FILE *fff;

	char file_name[1024];
	char skill_name[3][20]={_("マーシャルアーツ", "Martial Arts    "),
							_("二刀流          ", "Dual Wielding   "), 
							_("乗馬            ", "Riding          ")};

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	for (i = 0; i < 3; i++)
	{
		skill_exp = p_ptr->skill_exp[i];
		fprintf(fff, "%-20s ", skill_name[i]);
		if (skill_exp >= s_info[p_ptr->pclass].s_max[i]) fprintf(fff, "!");
		else fprintf(fff, " ");
		fprintf(fff, "%s", exp_level_str[(i == GINOU_RIDING) ? riding_exp_level(skill_exp) : weapon_exp_level(skill_exp)]);
		if (cheat_xtra) fprintf(fff, " %d", skill_exp);
		fprintf(fff, "\n");
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("技能の経験値", "Miscellaneous Proficiency"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*!
 * @brief 英単語、句、説を複数形を変換する / Pluralize a monster name
 * @param Name 変換したい文字列の参照ポインタ
 * @return なし
 */
void plural_aux(char *Name)
{
	int NameLen = strlen(Name);

	if (my_strstr(Name, "Disembodied hand"))
	{
		strcpy(Name, "Disembodied hands that strangled people");
	}
	else if (my_strstr(Name, "Colour out of space"))
	{
		strcpy(Name, "Colours out of space");
	}
	else if (my_strstr(Name, "stairway to hell"))
	{
		strcpy(Name, "stairways to hell");
	}
	else if (my_strstr(Name, "Dweller on the threshold"))
	{
		strcpy(Name, "Dwellers on the threshold");
	}
	else if (my_strstr(Name, " of "))
	{
		cptr aider = my_strstr(Name, " of ");
		char dummy[80];
		int i = 0;
		cptr ctr = Name;

		while (ctr < aider)
		{
			dummy[i] = *ctr;
			ctr++; i++;
		}

		if (dummy[i-1] == 's')
		{
			strcpy(&(dummy[i]), "es");
			i++;
		}
		else
		{
			strcpy(&(dummy[i]), "s");
		}

		strcpy(&(dummy[i+1]), aider);
		strcpy(Name, dummy);
	}
	else if (my_strstr(Name, "coins"))
	{
		char dummy[80];
		strcpy(dummy, "piles of ");
		strcat(dummy, Name);
		strcpy(Name, dummy);
		return;
	}
	else if (my_strstr(Name, "Manes"))
	{
		return;
	}
	else if (streq(&(Name[NameLen - 2]), "ey"))
	{
		strcpy(&(Name[NameLen - 2]), "eys");
	}
	else if (Name[NameLen - 1] == 'y')
	{
		strcpy(&(Name[NameLen - 1]), "ies");
	}
	else if (streq(&(Name[NameLen - 4]), "ouse"))
	{
		strcpy(&(Name[NameLen - 4]), "ice");
	}
	else if (streq(&(Name[NameLen - 2]), "us"))
	{
		strcpy(&(Name[NameLen - 2]), "i");
	}
	else if (streq(&(Name[NameLen - 6]), "kelman"))
	{
		strcpy(&(Name[NameLen - 6]), "kelmen");
	}
	else if (streq(&(Name[NameLen - 8]), "wordsman"))
	{
		strcpy(&(Name[NameLen - 8]), "wordsmen");
	}
	else if (streq(&(Name[NameLen - 7]), "oodsman"))
	{
		strcpy(&(Name[NameLen - 7]), "oodsmen");
	}
	else if (streq(&(Name[NameLen - 7]), "eastman"))
	{
		strcpy(&(Name[NameLen - 7]), "eastmen");
	}
	else if (streq(&(Name[NameLen - 8]), "izardman"))
	{
		strcpy(&(Name[NameLen - 8]), "izardmen");
	}
	else if (streq(&(Name[NameLen - 5]), "geist"))
	{
		strcpy(&(Name[NameLen - 5]), "geister");
	}
	else if (streq(&(Name[NameLen - 2]), "ex"))
	{
		strcpy(&(Name[NameLen - 2]), "ices");
	}
	else if (streq(&(Name[NameLen - 2]), "lf"))
	{
		strcpy(&(Name[NameLen - 2]), "lves");
	}
	else if (suffix(Name, "ch") ||
		 suffix(Name, "sh") ||
			 suffix(Name, "nx") ||
			 suffix(Name, "s") ||
			 suffix(Name, "o"))
	{
		strcpy(&(Name[NameLen]), "es");
	}
	else
	{
		strcpy(&(Name[NameLen]), "s");
	}
}

/*!
 * @brief 現在のペットを表示するコマンドのメインルーチン /
 * Display current pets
 * @return なし
 */
static void do_cmd_knowledge_pets(void)
{
	int             i;
	FILE            *fff;
	monster_type    *m_ptr;
	char            pet_name[80];
	int             t_friends = 0;
	int             show_upkeep = 0;
	char            file_name[1024];


	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		/* Calculate "upkeep" for pets */
		if (is_pet(m_ptr))
		{
			t_friends++;
			monster_desc(pet_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
			fprintf(fff, "%s (%s)\n", pet_name, look_mon_desc(m_ptr, 0x00));
		}
	}

	show_upkeep = calculate_upkeep();

	fprintf(fff, "----------------------------------------------\n");
#ifdef JP
	fprintf(fff, "    合計: %d 体のペット\n", t_friends);
	fprintf(fff, " 維持コスト: %d%% MP\n", show_upkeep);
#else
	fprintf(fff, "   Total: %d pet%s.\n",
		t_friends, (t_friends == 1 ? "" : "s"));
	fprintf(fff, "   Upkeep: %d%% mana.\n", show_upkeep);
#endif



	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("現在のペット", "Current Pets"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*!
 * @brief 現在のペットを表示するコマンドのメインルーチン /
 * Total kill count
 * @return なし
 * @note the player ghosts are ignored.  XXX XXX XXX
 */
static void do_cmd_knowledge_kill_count(void)
{
	int i, k, n = 0;
	u16b why = 2;
	s16b *who;

	FILE *fff;

	char file_name[1024];

	s32b Total = 0;


	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

	{
		/* Monsters slain */
		int kk;

		for (kk = 1; kk < max_r_idx; kk++)
		{
			monster_race *r_ptr = &r_info[kk];

			if (r_ptr->flags1 & (RF1_UNIQUE))
			{
				bool dead = (r_ptr->max_num == 0);

				if (dead)
				{
					Total++;
				}
			}
			else
			{
				s16b This = r_ptr->r_pkills;

				if (This > 0)
				{
					Total += This;
				}
			}
		}

		if (Total < 1)
			fprintf(fff,_("あなたはまだ敵を倒していない。\n\n", "You have defeated no enemies yet.\n\n"));
		else
#ifdef JP
			fprintf(fff,"あなたは%ld体の敵を倒している。\n\n", (long int)Total);
#else
			fprintf(fff,"You have defeated %ld %s.\n\n", (long int)Total, (Total == 1) ? "enemy" : "enemies");
#endif
	}

	Total = 0;

	/* Scan the monsters */
	for (i = 1; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Use that monster */
		if (r_ptr->name) who[n++] = i;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_hook;
	ang_sort_swap = ang_sort_swap_hook;

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, n);

	/* Scan the monster races */
	for (k = 0; k < n; k++)
	{
		monster_race *r_ptr = &r_info[who[k]];

		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			bool dead = (r_ptr->max_num == 0);

			if (dead)
			{
				/* Print a message */
				fprintf(fff, "     %s\n",
				    (r_name + r_ptr->name));
				Total++;
			}
		}
		else
		{
			s16b This = r_ptr->r_pkills;

			if (This > 0)
			{
#ifdef JP
				/* p,tは人と数える by ita */
				if (my_strchr("pt", r_ptr->d_char))
					fprintf(fff, "     %3d 人の %s\n", This, r_name + r_ptr->name);
				else
					fprintf(fff, "     %3d 体の %s\n", This, r_name + r_ptr->name);
#else
				if (This < 2)
				{
					if (my_strstr(r_name + r_ptr->name, "coins"))
					{
						fprintf(fff, "     1 pile of %s\n", (r_name + r_ptr->name));
					}
					else
					{
						fprintf(fff, "     1 %s\n", (r_name + r_ptr->name));
					}
				}
				else
				{
					char ToPlural[80];
					strcpy(ToPlural, (r_name + r_ptr->name));
					plural_aux(ToPlural);
					fprintf(fff, "     %d %s\n", This, ToPlural);
				}
#endif


				Total += This;
			}
		}
	}

	fprintf(fff,"----------------------------------------------\n");
#ifdef JP
	fprintf(fff,"    合計: %lu 体を倒した。\n", (unsigned long int)Total);
#else
	fprintf(fff,"   Total: %lu creature%s killed.\n",
		(unsigned long int)Total, (Total == 1 ? "" : "s"));
#endif


	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("倒した敵の数", "Kill Count"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*!
 * @brief モンスター情報リスト中のグループを表示する /
 * Display the object groups.
 * @param col 開始行
 * @param row 開始列
 * @param wid 表示文字数幅
 * @param per_page リストの表示行
 * @param grp_idx グループのID配列
 * @param group_text グループ名の文字列配列
 * @param grp_cur 現在の選択ID
 * @param grp_top 現在の選択リスト最上部ID
 * @return なし
 */
static void display_group_list(int col, int row, int wid, int per_page,
	int grp_idx[], cptr group_text[], int grp_cur, int grp_top)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && (grp_idx[i] >= 0); i++)
	{
		/* Get the group index */
		int grp = grp_idx[grp_top + i];

		/* Choose a color */
		byte attr = (grp_top + i == grp_cur) ? TERM_L_BLUE : TERM_WHITE;

		/* Erase the entire line */
		Term_erase(col, row + i, wid);

		/* Display the group label */
		c_put_str(attr, group_text[grp], row + i, col);
	}
}


/* 
 * Move the cursor in a browser window 
 */
static void browser_cursor(char ch, int *column, int *grp_cur, int grp_cnt, 
						   int *list_cur, int list_cnt)
{
	int d;
	int col = *column;
	int grp = *grp_cur;
	int list = *list_cur;

	/* Extract direction */
	if (ch == ' ')
	{
		/* Hack -- scroll up full screen */
		d = 3;
	}
	else if (ch == '-')
	{
		/* Hack -- scroll down full screen */
		d = 9;
	}
	else
	{
		d = get_keymap_dir(ch);
	}

	if (!d) return;

	/* Diagonals - hack */
	if ((ddx[d] > 0) && ddy[d])
	{
		int browser_rows;
		int wid, hgt;

		/* Get size */
		Term_get_size(&wid, &hgt);

		browser_rows = hgt - 8;

		/* Browse group list */
		if (!col)
		{
			int old_grp = grp;

			/* Move up or down */
			grp += ddy[d] * (browser_rows - 1);

			/* Verify */
			if (grp >= grp_cnt)	grp = grp_cnt - 1;
			if (grp < 0) grp = 0;
			if (grp != old_grp)	list = 0;
		}

		/* Browse sub-list list */
		else
		{
			/* Move up or down */
			list += ddy[d] * browser_rows;

			/* Verify */
			if (list >= list_cnt) list = list_cnt - 1;
			if (list < 0) list = 0;
		}

		(*grp_cur) = grp;
		(*list_cur) = list;

		return;
	}

	if (ddx[d])
	{
		col += ddx[d];
		if (col < 0) col = 0;
		if (col > 1) col = 1;

		(*column) = col;

		return;
	}

	/* Browse group list */
	if (!col)
	{
		int old_grp = grp;

		/* Move up or down */
		grp += ddy[d];

		/* Verify */
		if (grp >= grp_cnt)	grp = grp_cnt - 1;
		if (grp < 0) grp = 0;
		if (grp != old_grp)	list = 0;
	}

	/* Browse sub-list list */
	else
	{
		/* Move up or down */
		list += ddy[d];

		/* Verify */
		if (list >= list_cnt) list = list_cnt - 1;
		if (list < 0) list = 0;
	}

	(*grp_cur) = grp;
	(*list_cur) = list;
}


/*
 * Display visuals.
 */
static void display_visual_list(int col, int row, int height, int width, byte attr_top, byte char_left)
{
	int i, j;

	/* Clear the display lines */
	for (i = 0; i < height; i++)
	{
		Term_erase(col, row + i, width);
	}

	/* Bigtile mode uses double width */
	if (use_bigtile) width /= 2;

	/* Display lines until done */
	for (i = 0; i < height; i++)
	{
		/* Display columns until done */
		for (j = 0; j < width; j++)
		{
			byte a;
			char c;
			int x = col + j;
			int y = row + i;
			int ia, ic;

			/* Bigtile mode uses double width */
			if (use_bigtile) x += j;

			ia = attr_top + i;
			ic = char_left + j;

			/* Ignore illegal characters */
			if (ia > 0x7f || ic > 0xff || ic < ' ' ||
			    (!use_graphics && ic > 0x7f))
				continue;

			a = (byte)ia;
			c = (char)ic;

			/* Force correct code for both ASCII character and tile */
			if (c & 0x80) a |= 0x80;

			/* Display symbol */
			Term_queue_bigchar(x, y, a, c, 0, 0);
		}
	}
}


/*
 * Place the cursor at the collect position for visual mode
 */
static void place_visual_list_cursor(int col, int row, byte a, byte c, byte attr_top, byte char_left)
{
	int i = (a & 0x7f) - attr_top;
	int j = c - char_left;

	int x = col + j;
	int y = row + i;

	/* Bigtile mode uses double width */
	if (use_bigtile) x += j;

	/* Place the cursor */
	Term_gotoxy(x, y);
}


/*
 *  Clipboard variables for copy&paste in visual mode
 */
static byte attr_idx = 0;
static byte char_idx = 0;

/* Hack -- for feature lighting */
static byte attr_idx_feat[F_LIT_MAX];
static byte char_idx_feat[F_LIT_MAX];

/*
 *  Do visual mode command -- Change symbols
 */
static bool visual_mode_command(char ch, bool *visual_list_ptr,
				int height, int width,
				byte *attr_top_ptr, byte *char_left_ptr,
				byte *cur_attr_ptr, byte *cur_char_ptr, bool *need_redraw)
{
	static byte attr_old = 0, char_old = 0;

	switch (ch)
	{
	case ESCAPE:
		if (*visual_list_ptr)
		{
			/* Cancel change */
			*cur_attr_ptr = attr_old;
			*cur_char_ptr = char_old;
			*visual_list_ptr = FALSE;

			return TRUE;
		}
		break;

	case '\n':
	case '\r':
		if (*visual_list_ptr)
		{
			/* Accept change */
			*visual_list_ptr = FALSE;
			*need_redraw = TRUE;

			return TRUE;
		}
		break;

	case 'V':
	case 'v':
		if (!*visual_list_ptr)
		{
			*visual_list_ptr = TRUE;

			*attr_top_ptr = MAX(0, (*cur_attr_ptr & 0x7f) - 5);
			*char_left_ptr = MAX(0, *cur_char_ptr - 10);

			attr_old = *cur_attr_ptr;
			char_old = *cur_char_ptr;

			return TRUE;
		}
		break;

	case 'C':
	case 'c':
		{
			int i;

			/* Set the visual */
			attr_idx = *cur_attr_ptr;
			char_idx = *cur_char_ptr;

			/* Hack -- for feature lighting */
			for (i = 0; i < F_LIT_MAX; i++)
			{
				attr_idx_feat[i] = 0;
				char_idx_feat[i] = 0;
			}
		}
		return TRUE;

	case 'P':
	case 'p':
		if (attr_idx || (!(char_idx & 0x80) && char_idx)) /* Allow TERM_DARK text */
		{
			/* Set the char */
			*cur_attr_ptr = attr_idx;
			*attr_top_ptr = MAX(0, (*cur_attr_ptr & 0x7f) - 5);
			if (!*visual_list_ptr) *need_redraw = TRUE;
		}

		if (char_idx)
		{
			/* Set the char */
			*cur_char_ptr = char_idx;
			*char_left_ptr = MAX(0, *cur_char_ptr - 10);
			if (!*visual_list_ptr) *need_redraw = TRUE;
		}

		return TRUE;

	default:
		if (*visual_list_ptr)
		{
			int eff_width;
			int d = get_keymap_dir(ch);
			byte a = (*cur_attr_ptr & 0x7f);
			byte c = *cur_char_ptr;

			if (use_bigtile) eff_width = width / 2;
			else eff_width = width;

			/* Restrict direction */
			if ((a == 0) && (ddy[d] < 0)) d = 0;
			if ((c == 0) && (ddx[d] < 0)) d = 0;
			if ((a == 0x7f) && (ddy[d] > 0)) d = 0;
			if ((c == 0xff) && (ddx[d] > 0)) d = 0;

			a += ddy[d];
			c += ddx[d];

			/* Force correct code for both ASCII character and tile */
			if (c & 0x80) a |= 0x80;

			/* Set the visual */
			*cur_attr_ptr = a;
			*cur_char_ptr = c;


			/* Move the frame */
			if ((ddx[d] < 0) && *char_left_ptr > MAX(0, (int)c - 10)) (*char_left_ptr)--;
			if ((ddx[d] > 0) && *char_left_ptr + eff_width < MIN(0xff, (int)c + 10)) (*char_left_ptr)++;
			if ((ddy[d] < 0) && *attr_top_ptr > MAX(0, (int)(a & 0x7f) - 4)) (*attr_top_ptr)--;
			if ((ddy[d] > 0) && *attr_top_ptr + height < MIN(0x7f, (a & 0x7f) + 4)) (*attr_top_ptr)++;
			return TRUE;
		}
		break;
	}

	/* Visual mode command is not used */
	return FALSE;
}


/*
 * Display the monsters in a group.
 */
static void display_monster_list(int col, int row, int per_page, s16b mon_idx[],
	int mon_cur, int mon_top, bool visual_only)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && (mon_idx[mon_top + i] >= 0); i++)
	{
		byte attr;

		/* Get the race index */
		int r_idx = mon_idx[mon_top + i] ;

		/* Access the race */
		monster_race *r_ptr = &r_info[r_idx];

		/* Choose a color */
		attr = ((i + mon_top == mon_cur) ? TERM_L_BLUE : TERM_WHITE);

		/* Display the name */
		c_prt(attr, (r_name + r_ptr->name), row + i, col);

		/* Hack -- visual_list mode */
		if (per_page == 1)
		{
			c_prt(attr, format("%02x/%02x", r_ptr->x_attr, r_ptr->x_char), row + i, (p_ptr->wizard || visual_only) ? 56 : 61);
		}
		if (p_ptr->wizard || visual_only)
		{
			c_prt(attr, format("%d", r_idx), row + i, 62);
		}

		/* Erase chars before overwritten by the race letter */
		Term_erase(69, row + i, 255);

		/* Display symbol */
		Term_queue_bigchar(use_bigtile ? 69 : 70, row + i, r_ptr->x_attr, r_ptr->x_char, 0, 0);

		if (!visual_only)
		{
			/* Display kills */
			if (!(r_ptr->flags1 & RF1_UNIQUE)) 
				put_str(format("%5d", r_ptr->r_pkills), row + i, 73);
			else 
				c_put_str((r_ptr->max_num == 0 ? TERM_L_DARK : TERM_WHITE), 
						  (r_ptr->max_num == 0 ? _("死亡", " dead") : _("生存", "alive")), row + i, 74);
		}
	}

	/* Clear remaining lines */
	for (; i < per_page; i++)
	{
		Term_erase(col, row + i, 255);
	}
}


/*
 * Display known monsters.
 */
static void do_cmd_knowledge_monsters(bool *need_redraw, bool visual_only, int direct_r_idx)
{
	int i, len, max;
	int grp_cur, grp_top, old_grp_cur;
	int mon_cur, mon_top;
	int grp_cnt, grp_idx[100];
	int mon_cnt;
	s16b *mon_idx;

	int column = 0;
	bool flag;
	bool redraw;

	bool visual_list = FALSE;
	byte attr_top = 0, char_left = 0;

	int browser_rows;
	int wid, hgt;

	byte mode;

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "mon_idx" array */
	C_MAKE(mon_idx, max_r_idx, s16b);

	max = 0;
	grp_cnt = 0;

	if (direct_r_idx < 0)
	{
		mode = visual_only ? 0x03 : 0x01;

		/* Check every group */
		for (i = 0; monster_group_text[i] != NULL; i++)
		{
			/* Measure the label */
			len = strlen(monster_group_text[i]);

			/* Save the maximum length */
			if (len > max) max = len;

			/* See if any monsters are known */
			if ((monster_group_char[i] == ((char *) -1L)) || collect_monsters(i, mon_idx, mode))
			{
				/* Build a list of groups with known monsters */
				grp_idx[grp_cnt++] = i;
			}
		}

		mon_cnt = 0;
	}
	else
	{
		mon_idx[0] = direct_r_idx;
		mon_cnt = 1;

		/* Terminate the list */
		mon_idx[1] = -1;

		(void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - (max + 3),
			&attr_top, &char_left, &r_info[direct_r_idx].x_attr, &r_info[direct_r_idx].x_char, need_redraw);
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	mon_cur = mon_top = 0;

	flag = FALSE;
	redraw = TRUE;

	mode = visual_only ? 0x02 : 0x00;

	while (!flag)
	{
		char ch;
		monster_race *r_ptr;

		if (redraw)
		{
			clear_from(0);

#ifdef JP
			prt(format("%s - モンスター", !visual_only ? "知識" : "表示"), 2, 0);
			if (direct_r_idx < 0) prt("グループ", 4, 0);
			prt("名前", 4, max + 3);
			if (p_ptr->wizard || visual_only) prt("Idx", 4, 62);
			prt("文字", 4, 67);
			if (!visual_only) prt("殺害数", 4, 72);
#else
			prt(format("%s - monsters", !visual_only ? "Knowledge" : "Visuals"), 2, 0);
			if (direct_r_idx < 0) prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			if (p_ptr->wizard || visual_only) prt("Idx", 4, 62);
			prt("Sym", 4, 68);
			if (!visual_only) prt("Kills", 4, 73);
#endif

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			if (direct_r_idx < 0)
			{
				for (i = 0; i < browser_rows; i++)
				{
					Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
				}
			}

			redraw = FALSE;
		}

		if (direct_r_idx < 0)
		{
			/* Scroll group list */
			if (grp_cur < grp_top) grp_top = grp_cur;
			if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

			/* Display a list of monster groups */
			display_group_list(0, 6, max, browser_rows, grp_idx, monster_group_text, grp_cur, grp_top);

			if (old_grp_cur != grp_cur)
			{
				old_grp_cur = grp_cur;

				/* Get a list of monsters in the current group */
				mon_cnt = collect_monsters(grp_idx[grp_cur], mon_idx, mode);
			}

			/* Scroll monster list */
			while (mon_cur < mon_top)
				mon_top = MAX(0, mon_top - browser_rows/2);
			while (mon_cur >= mon_top + browser_rows)
				mon_top = MIN(mon_cnt - browser_rows, mon_top + browser_rows/2);
		}

		if (!visual_list)
		{
			/* Display a list of monsters in the current group */
			display_monster_list(max + 3, 6, browser_rows, mon_idx, mon_cur, mon_top, visual_only);
		}
		else
		{
			mon_top = mon_cur;

			/* Display a monster name */
			display_monster_list(max + 3, 6, 1, mon_idx, mon_cur, mon_top, visual_only);

			/* Display visual list below first monster */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Prompt */
#ifdef JP
		prt(format("<方向>%s%s%s, ESC",
			(!visual_list && !visual_only) ? ", 'r'で思い出を見る" : "",
			visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更",
			(attr_idx || char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
			hgt - 1, 0);
#else
		prt(format("<dir>%s%s%s, ESC",
			(!visual_list && !visual_only) ? ", 'r' to recall" : "",
			visual_list ? ", ENTER to accept" : ", 'v' for visuals",
			(attr_idx || char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
			hgt - 1, 0);
#endif

		/* Get the current monster */
		r_ptr = &r_info[mon_idx[mon_cur]];

		if (!visual_only)
		{
			/* Mega Hack -- track this monster race */
			if (mon_cnt) monster_race_track(mon_idx[mon_cur]);

			/* Hack -- handle stuff */
			handle_stuff();
		}

		if (visual_list)
		{
			place_visual_list_cursor(max + 3, 7, r_ptr->x_attr, r_ptr->x_char, attr_top, char_left);
		}
		else if (!column)
		{
			Term_gotoxy(0, 6 + (grp_cur - grp_top));
		}
		else
		{
			Term_gotoxy(max + 3, 6 + (mon_cur - mon_top));
		}

		ch = inkey();

		/* Do visual mode command if needed */
		if (visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, &r_ptr->x_attr, &r_ptr->x_char, need_redraw))
		{
			if (direct_r_idx >= 0)
			{
				switch (ch)
				{
				case '\n':
				case '\r':
				case ESCAPE:
					flag = TRUE;
					break;
				}
			}
			continue;
		}

		switch (ch)
		{
			case ESCAPE:
			{
				flag = TRUE;
				break;
			}

			case 'R':
			case 'r':
			{
				/* Recall on screen */
				if (!visual_list && !visual_only && (mon_idx[mon_cur] > 0))
				{
					screen_roff(mon_idx[mon_cur], 0);

					(void)inkey();

					redraw = TRUE;
				}
				break;
			}

			default:
			{
				/* Move the cursor */
				browser_cursor(ch, &column, &grp_cur, grp_cnt, &mon_cur, mon_cnt);

				break;
			}
		}
	}

	/* Free the "mon_idx" array */
	C_KILL(mon_idx, max_r_idx, s16b);
}


/*
 * Display the objects in a group.
 */
static void display_object_list(int col, int row, int per_page, int object_idx[],
	int object_cur, int object_top, bool visual_only)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && (object_idx[object_top + i] >= 0); i++)
	{
		char o_name[80];
		byte a, c;
		object_kind *flavor_k_ptr;

		/* Get the object index */
		int k_idx = object_idx[object_top + i];

		/* Access the object */
		object_kind *k_ptr = &k_info[k_idx];

		/* Choose a color */
		byte attr = ((k_ptr->aware || visual_only) ? TERM_WHITE : TERM_SLATE);
		byte cursor = ((k_ptr->aware || visual_only) ? TERM_L_BLUE : TERM_BLUE);


		if (!visual_only && k_ptr->flavor)
		{
			/* Appearance of this object is shuffled */
			flavor_k_ptr = &k_info[k_ptr->flavor];
		}
		else
		{
			/* Appearance of this object is very normal */
			flavor_k_ptr = k_ptr;
		}



		attr = ((i + object_top == object_cur) ? cursor : attr);

		if (!k_ptr->flavor || (!visual_only && k_ptr->aware))
		{
			/* Tidy name */
			strip_name(o_name, k_idx);
		}
		else
		{
			/* Flavor name */
			strcpy(o_name, k_name + flavor_k_ptr->flavor_name);
		}

		/* Display the name */
		c_prt(attr, o_name, row + i, col);

		/* Hack -- visual_list mode */
		if (per_page == 1)
		{
			c_prt(attr, format("%02x/%02x", flavor_k_ptr->x_attr, flavor_k_ptr->x_char), row + i, (p_ptr->wizard || visual_only) ? 64 : 68);
		}
		if (p_ptr->wizard || visual_only)
		{
			c_prt(attr, format("%d", k_idx), row + i, 70);
		}

		a = flavor_k_ptr->x_attr;
		c = flavor_k_ptr->x_char;

		/* Display symbol */
		Term_queue_bigchar(use_bigtile ? 76 : 77, row + i, a, c, 0, 0);
	}

	/* Clear remaining lines */
	for (; i < per_page; i++)
	{
		Term_erase(col, row + i, 255);
	}
}

/*
 * Describe fake object
 */
static void desc_obj_fake(int k_idx)
{
	object_type *o_ptr;
	object_type object_type_body;

	/* Get local object */
	o_ptr = &object_type_body;

	/* Wipe the object */
	object_wipe(o_ptr);

	/* Create the artifact */
	object_prep(o_ptr, k_idx);

	/* It's fully know */
	o_ptr->ident |= IDENT_KNOWN;

	/* Track the object */
	/* object_actual_track(o_ptr); */

	/* Hack - mark as fake */
	/* term_obj_real = FALSE; */

	/* Hack -- Handle stuff */
	handle_stuff();

	if (!screen_object(o_ptr, SCROBJ_FAKE_OBJECT | SCROBJ_FORCE_DETAIL))
	{
		msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
		msg_print(NULL);
	}
}



/*
 * Display known objects
 */
static void do_cmd_knowledge_objects(bool *need_redraw, bool visual_only, int direct_k_idx)
{
	int i, len, max;
	int grp_cur, grp_top, old_grp_cur;
	int object_old, object_cur, object_top;
	int grp_cnt, grp_idx[100];
	int object_cnt;
	int *object_idx;

	int column = 0;
	bool flag;
	bool redraw;

	bool visual_list = FALSE;
	byte attr_top = 0, char_left = 0;

	int browser_rows;
	int wid, hgt;

	byte mode;

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "object_idx" array */
	C_MAKE(object_idx, max_k_idx, int);

	max = 0;
	grp_cnt = 0;

	if (direct_k_idx < 0)
	{
		mode = visual_only ? 0x03 : 0x01;

		/* Check every group */
		for (i = 0; object_group_text[i] != NULL; i++)
		{
			/* Measure the label */
			len = strlen(object_group_text[i]);

			/* Save the maximum length */
			if (len > max) max = len;

			/* See if any monsters are known */
			if (collect_objects(i, object_idx, mode))
			{
				/* Build a list of groups with known monsters */
				grp_idx[grp_cnt++] = i;
			}
		}

		object_old = -1;
		object_cnt = 0;
	}
	else
	{
		object_kind *k_ptr = &k_info[direct_k_idx];
		object_kind *flavor_k_ptr;

		if (!visual_only && k_ptr->flavor)
		{
			/* Appearance of this object is shuffled */
			flavor_k_ptr = &k_info[k_ptr->flavor];
		}
		else
		{
			/* Appearance of this object is very normal */
			flavor_k_ptr = k_ptr;
		}

		object_idx[0] = direct_k_idx;
		object_old = direct_k_idx;
		object_cnt = 1;

		/* Terminate the list */
		object_idx[1] = -1;

		(void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - (max + 3),
			&attr_top, &char_left, &flavor_k_ptr->x_attr, &flavor_k_ptr->x_char, need_redraw);
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	object_cur = object_top = 0;

	flag = FALSE;
	redraw = TRUE;

	mode = visual_only ? 0x02 : 0x00;

	while (!flag)
	{
		char ch;
		object_kind *k_ptr, *flavor_k_ptr;

		if (redraw)
		{
			clear_from(0);

#ifdef JP
			prt(format("%s - アイテム", !visual_only ? "知識" : "表示"), 2, 0);
			if (direct_k_idx < 0) prt("グループ", 4, 0);
			prt("名前", 4, max + 3);
			if (p_ptr->wizard || visual_only) prt("Idx", 4, 70);
			prt("文字", 4, 74);
#else
			prt(format("%s - objects", !visual_only ? "Knowledge" : "Visuals"), 2, 0);
			if (direct_k_idx < 0) prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			if (p_ptr->wizard || visual_only) prt("Idx", 4, 70);
			prt("Sym", 4, 75);
#endif

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			if (direct_k_idx < 0)
			{
				for (i = 0; i < browser_rows; i++)
				{
					Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
				}
			}

			redraw = FALSE;
		}

		if (direct_k_idx < 0)
		{
			/* Scroll group list */
			if (grp_cur < grp_top) grp_top = grp_cur;
			if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

			/* Display a list of object groups */
			display_group_list(0, 6, max, browser_rows, grp_idx, object_group_text, grp_cur, grp_top);

			if (old_grp_cur != grp_cur)
			{
				old_grp_cur = grp_cur;

				/* Get a list of objects in the current group */
				object_cnt = collect_objects(grp_idx[grp_cur], object_idx, mode);
			}

			/* Scroll object list */
			while (object_cur < object_top)
				object_top = MAX(0, object_top - browser_rows/2);
			while (object_cur >= object_top + browser_rows)
				object_top = MIN(object_cnt - browser_rows, object_top + browser_rows/2);
		}

		if (!visual_list)
		{
			/* Display a list of objects in the current group */
			display_object_list(max + 3, 6, browser_rows, object_idx, object_cur, object_top, visual_only);
		}
		else
		{
			object_top = object_cur;

			/* Display a list of objects in the current group */
			display_object_list(max + 3, 6, 1, object_idx, object_cur, object_top, visual_only);

			/* Display visual list below first object */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Get the current object */
		k_ptr = &k_info[object_idx[object_cur]];

		if (!visual_only && k_ptr->flavor)
		{
			/* Appearance of this object is shuffled */
			flavor_k_ptr = &k_info[k_ptr->flavor];
		}
		else
		{
			/* Appearance of this object is very normal */
			flavor_k_ptr = k_ptr;
		}

		/* Prompt */
#ifdef JP
		prt(format("<方向>%s%s%s, ESC",
			(!visual_list && !visual_only) ? ", 'r'で詳細を見る" : "",
			visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更",
			(attr_idx || char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
			hgt - 1, 0);
#else
		prt(format("<dir>%s%s%s, ESC",
			(!visual_list && !visual_only) ? ", 'r' to recall" : "",
			visual_list ? ", ENTER to accept" : ", 'v' for visuals",
			(attr_idx || char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
			hgt - 1, 0);
#endif

		if (!visual_only)
		{
			/* Mega Hack -- track this object */
			if (object_cnt) object_kind_track(object_idx[object_cur]);

			/* The "current" object changed */
			if (object_old != object_idx[object_cur])
			{
				/* Hack -- handle stuff */
				handle_stuff();

				/* Remember the "current" object */
				object_old = object_idx[object_cur];
			}
		}

		if (visual_list)
		{
			place_visual_list_cursor(max + 3, 7, flavor_k_ptr->x_attr, flavor_k_ptr->x_char, attr_top, char_left);
		}
		else if (!column)
		{
			Term_gotoxy(0, 6 + (grp_cur - grp_top));
		}
		else
		{
			Term_gotoxy(max + 3, 6 + (object_cur - object_top));
		}

		ch = inkey();

		/* Do visual mode command if needed */
		if (visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, &flavor_k_ptr->x_attr, &flavor_k_ptr->x_char, need_redraw))
		{
			if (direct_k_idx >= 0)
			{
				switch (ch)
				{
				case '\n':
				case '\r':
				case ESCAPE:
					flag = TRUE;
					break;
				}
			}
			continue;
		}

		switch (ch)
		{
			case ESCAPE:
			{
				flag = TRUE;
				break;
			}

			case 'R':
			case 'r':
			{
				/* Recall on screen */
				if (!visual_list && !visual_only && (grp_cnt > 0))
				{
					desc_obj_fake(object_idx[object_cur]);
					redraw = TRUE;
				}
				break;
			}

			default:
			{
				/* Move the cursor */
				browser_cursor(ch, &column, &grp_cur, grp_cnt, &object_cur, object_cnt);
				break;
			}
		}
	}

	/* Free the "object_idx" array */
	C_KILL(object_idx, max_k_idx, int);
}


/*
 * Display the features in a group.
 */
static void display_feature_list(int col, int row, int per_page, int *feat_idx,
	int feat_cur, int feat_top, bool visual_only, int lighting_level)
{
	int lit_col[F_LIT_MAX], i, j;
	int f_idx_col = use_bigtile ? 62 : 64;

	/* Correct columns 1 and 4 */
	lit_col[F_LIT_STANDARD] = use_bigtile ? (71 - F_LIT_MAX) : 71;
	for (i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
		lit_col[i] = lit_col[F_LIT_STANDARD] + 2 + (i - F_LIT_NS_BEGIN) * 2 + (use_bigtile ? i : 0);

	/* Display lines until done */
	for (i = 0; i < per_page && (feat_idx[feat_top + i] >= 0); i++)
	{
		byte attr;

		/* Get the index */
		int f_idx = feat_idx[feat_top + i];

		/* Access the index */
		feature_type *f_ptr = &f_info[f_idx];

		int row_i = row + i;

		/* Choose a color */
		attr = ((i + feat_top == feat_cur) ? TERM_L_BLUE : TERM_WHITE);

		/* Display the name */
		c_prt(attr, f_name + f_ptr->name, row_i, col);

		/* Hack -- visual_list mode */
		if (per_page == 1)
		{
			/* Display lighting level */
			c_prt(attr, format("(%s)", lighting_level_str[lighting_level]), row_i, col + 1 + strlen(f_name + f_ptr->name));

			c_prt(attr, format("%02x/%02x", f_ptr->x_attr[lighting_level], f_ptr->x_char[lighting_level]), row_i, f_idx_col - ((p_ptr->wizard || visual_only) ? 6 : 2));
		}
		if (p_ptr->wizard || visual_only)
		{
			c_prt(attr, format("%d", f_idx), row_i, f_idx_col);
		}

		/* Display symbol */
		Term_queue_bigchar(lit_col[F_LIT_STANDARD], row_i, f_ptr->x_attr[F_LIT_STANDARD], f_ptr->x_char[F_LIT_STANDARD], 0, 0);

		Term_putch(lit_col[F_LIT_NS_BEGIN], row_i, TERM_SLATE, '(');
		for (j = F_LIT_NS_BEGIN + 1; j < F_LIT_MAX; j++)
		{
			Term_putch(lit_col[j], row_i, TERM_SLATE, '/');
		}
		Term_putch(lit_col[F_LIT_MAX - 1] + (use_bigtile ? 3 : 2), row_i, TERM_SLATE, ')');

		/* Mega-hack -- Use non-standard colour */
		for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
		{
			Term_queue_bigchar(lit_col[j] + 1, row_i, f_ptr->x_attr[j], f_ptr->x_char[j], 0, 0);
		}
	}

	/* Clear remaining lines */
	for (; i < per_page; i++)
	{
		Term_erase(col, row + i, 255);
	}
}


/*
 * Interact with feature visuals.
 */
static void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, int direct_f_idx, int *lighting_level)
{
	int i, len, max;
	int grp_cur, grp_top, old_grp_cur;
	int feat_cur, feat_top;
	int grp_cnt, grp_idx[100];
	int feat_cnt;
	int *feat_idx;

	int column = 0;
	bool flag;
	bool redraw;

	bool visual_list = FALSE;
	byte attr_top = 0, char_left = 0;

	int browser_rows;
	int wid, hgt;

	byte attr_old[F_LIT_MAX];
	byte char_old[F_LIT_MAX];
	byte *cur_attr_ptr, *cur_char_ptr;

	(void)C_WIPE(attr_old, F_LIT_MAX, byte);
	(void)C_WIPE(char_old, F_LIT_MAX, byte);

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "feat_idx" array */
	C_MAKE(feat_idx, max_f_idx, int);

	max = 0;
	grp_cnt = 0;

	if (direct_f_idx < 0)
	{
		/* Check every group */
		for (i = 0; feature_group_text[i] != NULL; i++)
		{
			/* Measure the label */
			len = strlen(feature_group_text[i]);

			/* Save the maximum length */
			if (len > max) max = len;

			/* See if any features are known */
			if (collect_features(i, feat_idx, 0x01))
			{
				/* Build a list of groups with known features */
				grp_idx[grp_cnt++] = i;
			}
		}

		feat_cnt = 0;
	}
	else
	{
		feature_type *f_ptr = &f_info[direct_f_idx];

		feat_idx[0] = direct_f_idx;
		feat_cnt = 1;

		/* Terminate the list */
		feat_idx[1] = -1;

		(void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - (max + 3),
			&attr_top, &char_left, &f_ptr->x_attr[*lighting_level], &f_ptr->x_char[*lighting_level], need_redraw);

		for (i = 0; i < F_LIT_MAX; i++)
		{
			attr_old[i] = f_ptr->x_attr[i];
			char_old[i] = f_ptr->x_char[i];
		}
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	feat_cur = feat_top = 0;

	flag = FALSE;
	redraw = TRUE;

	while (!flag)
	{
		char ch;
		feature_type *f_ptr;

		if (redraw)
		{
			clear_from(0);

#ifdef JP
			prt("表示 - 地形", 2, 0);
			if (direct_f_idx < 0) prt("グループ", 4, 0);
			prt("名前", 4, max + 3);
			if (use_bigtile)
			{
				if (p_ptr->wizard || visual_only) prt("Idx", 4, 62);
				prt("文字 ( l/ d)", 4, 66);
			}
			else
			{
				if (p_ptr->wizard || visual_only) prt("Idx", 4, 64);
				prt("文字 (l/d)", 4, 68);
			}
#else
			prt("Visuals - features", 2, 0);
			if (direct_f_idx < 0) prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			if (use_bigtile)
			{
				if (p_ptr->wizard || visual_only) prt("Idx", 4, 62);
				prt("Sym ( l/ d)", 4, 67);
			}
			else
			{
				if (p_ptr->wizard || visual_only) prt("Idx", 4, 64);
				prt("Sym (l/d)", 4, 69);
			}
#endif

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			if (direct_f_idx < 0)
			{
				for (i = 0; i < browser_rows; i++)
				{
					Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
				}
			}

			redraw = FALSE;
		}

		if (direct_f_idx < 0)
		{
			/* Scroll group list */
			if (grp_cur < grp_top) grp_top = grp_cur;
			if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

			/* Display a list of feature groups */
			display_group_list(0, 6, max, browser_rows, grp_idx, feature_group_text, grp_cur, grp_top);

			if (old_grp_cur != grp_cur)
			{
				old_grp_cur = grp_cur;

				/* Get a list of features in the current group */
				feat_cnt = collect_features(grp_idx[grp_cur], feat_idx, 0x00);
			}

			/* Scroll feature list */
			while (feat_cur < feat_top)
				feat_top = MAX(0, feat_top - browser_rows/2);
			while (feat_cur >= feat_top + browser_rows)
				feat_top = MIN(feat_cnt - browser_rows, feat_top + browser_rows/2);
		}

		if (!visual_list)
		{
			/* Display a list of features in the current group */
			display_feature_list(max + 3, 6, browser_rows, feat_idx, feat_cur, feat_top, visual_only, F_LIT_STANDARD);
		}
		else
		{
			feat_top = feat_cur;

			/* Display a list of features in the current group */
			display_feature_list(max + 3, 6, 1, feat_idx, feat_cur, feat_top, visual_only, *lighting_level);

			/* Display visual list below first object */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Prompt */
#ifdef JP
		prt(format("<方向>%s, 'd'で標準光源効果%s, ESC",
			visual_list ? ", ENTERで決定, 'a'で対象明度変更" : ", 'v'でシンボル変更",
			(attr_idx || char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
			hgt - 1, 0);
#else
		prt(format("<dir>%s, 'd' for default lighting%s, ESC",
			visual_list ? ", ENTER to accept, 'a' for lighting level" : ", 'v' for visuals",
			(attr_idx || char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
			hgt - 1, 0);
#endif

		/* Get the current feature */
		f_ptr = &f_info[feat_idx[feat_cur]];
		cur_attr_ptr = &f_ptr->x_attr[*lighting_level];
		cur_char_ptr = &f_ptr->x_char[*lighting_level];

		if (visual_list)
		{
			place_visual_list_cursor(max + 3, 7, *cur_attr_ptr, *cur_char_ptr, attr_top, char_left);
		}
		else if (!column)
		{
			Term_gotoxy(0, 6 + (grp_cur - grp_top));
		}
		else
		{
			Term_gotoxy(max + 3, 6 + (feat_cur - feat_top));
		}

		ch = inkey();

		if (visual_list && ((ch == 'A') || (ch == 'a')))
		{
			int prev_lighting_level = *lighting_level;

			if (ch == 'A')
			{
				if (*lighting_level <= 0) *lighting_level = F_LIT_MAX - 1;
				else (*lighting_level)--;
			}
			else
			{
				if (*lighting_level >= F_LIT_MAX - 1) *lighting_level = 0;
				else (*lighting_level)++;
			}

			if (f_ptr->x_attr[prev_lighting_level] != f_ptr->x_attr[*lighting_level])
				attr_top = MAX(0, (f_ptr->x_attr[*lighting_level] & 0x7f) - 5);

			if (f_ptr->x_char[prev_lighting_level] != f_ptr->x_char[*lighting_level])
				char_left = MAX(0, f_ptr->x_char[*lighting_level] - 10);

			continue;
		}

		else if ((ch == 'D') || (ch == 'd'))
		{
			byte prev_x_attr = f_ptr->x_attr[*lighting_level];
			byte prev_x_char = f_ptr->x_char[*lighting_level];

			apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);

			if (visual_list)
			{
				if (prev_x_attr != f_ptr->x_attr[*lighting_level])
					 attr_top = MAX(0, (f_ptr->x_attr[*lighting_level] & 0x7f) - 5);

				if (prev_x_char != f_ptr->x_char[*lighting_level])
					char_left = MAX(0, f_ptr->x_char[*lighting_level] - 10);
			}
			else *need_redraw = TRUE;

			continue;
		}

		/* Do visual mode command if needed */
		else if (visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, cur_attr_ptr, cur_char_ptr, need_redraw))
		{
			switch (ch)
			{
			/* Restore previous visual settings */
			case ESCAPE:
				for (i = 0; i < F_LIT_MAX; i++)
				{
					f_ptr->x_attr[i] = attr_old[i];
					f_ptr->x_char[i] = char_old[i];
				}

				/* Fall through */

			case '\n':
			case '\r':
				if (direct_f_idx >= 0) flag = TRUE;
				else *lighting_level = F_LIT_STANDARD;
				break;

			/* Preserve current visual settings */
			case 'V':
			case 'v':
				for (i = 0; i < F_LIT_MAX; i++)
				{
					attr_old[i] = f_ptr->x_attr[i];
					char_old[i] = f_ptr->x_char[i];
				}
				*lighting_level = F_LIT_STANDARD;
				break;

			case 'C':
			case 'c':
				if (!visual_list)
				{
					for (i = 0; i < F_LIT_MAX; i++)
					{
						attr_idx_feat[i] = f_ptr->x_attr[i];
						char_idx_feat[i] = f_ptr->x_char[i];
					}
				}
				break;

			case 'P':
			case 'p':
				if (!visual_list)
				{
					/* Allow TERM_DARK text */
					for (i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
					{
						if (attr_idx_feat[i] || (!(char_idx_feat[i] & 0x80) && char_idx_feat[i])) f_ptr->x_attr[i] = attr_idx_feat[i];
						if (char_idx_feat[i]) f_ptr->x_char[i] = char_idx_feat[i];
					}
				}
				break;
			}
			continue;
		}

		switch (ch)
		{
			case ESCAPE:
			{
				flag = TRUE;
				break;
			}

			default:
			{
				/* Move the cursor */
				browser_cursor(ch, &column, &grp_cur, grp_cnt, &feat_cur, feat_cnt);
				break;
			}
		}
	}

	/* Free the "feat_idx" array */
	C_KILL(feat_idx, max_f_idx, int);
}


/*
 * List wanted monsters
 */
static void do_cmd_knowledge_kubi(void)
{
	int i;
	FILE *fff;
	
	char file_name[1024];
	
	
	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}
	
	if (fff)
	{
		bool listed = FALSE;

#ifdef JP
		fprintf(fff, "今日のターゲット : %s\n", (p_ptr->today_mon ? r_name + r_info[p_ptr->today_mon].name : "不明"));
		fprintf(fff, "\n");
		fprintf(fff, "賞金首リスト\n");
#else
		fprintf(fff, "Today target : %s\n", (p_ptr->today_mon ? r_name + r_info[p_ptr->today_mon].name : "unknown"));
		fprintf(fff, "\n");
		fprintf(fff, "List of wanted monsters\n");
#endif
		fprintf(fff, "----------------------------------------------\n");

		for (i = 0; i < MAX_KUBI; i++)
		{
			if (kubi_r_idx[i] <= 10000)
			{
				fprintf(fff,"%s\n", r_name + r_info[kubi_r_idx[i]].name);

				listed = TRUE;
			}
		}

		if (!listed)
		{
			fprintf(fff,"\n%s\n", _("賞金首はもう残っていません。", "There is no more wanted monster."));
		}
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
	show_file(TRUE, file_name, _("賞金首の一覧", "Wanted monsters"), 0, 0);
	
	/* Remove the file */
	fd_kill(file_name);
}

/*
 * List virtues & status
 */
static void do_cmd_knowledge_virtues(void)
{
	FILE *fff;
	
	char file_name[1024];
	
	
	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}
	
	if (fff)
	{
		fprintf(fff, _("現在の属性 : %s\n\n", "Your alighnment : %s\n\n"), your_alignment());
		dump_virtues(fff);
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
	show_file(TRUE, file_name, _("八つの徳", "Virtues"), 0, 0);
	
	/* Remove the file */
	fd_kill(file_name);
}

/*
* Dungeon
*
*/
static void do_cmd_knowledge_dungeon(void)
{
	FILE *fff;
	
	char file_name[1024];
	int i;
	
	
	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}
	
	if (fff)
	{
		for (i = 1; i < max_d_idx; i++)
		{
			bool seiha = FALSE;

			if (!d_info[i].maxdepth) continue;
			if (!max_dlv[i]) continue;
			if (d_info[i].final_guardian)
			{
				if (!r_info[d_info[i].final_guardian].max_num) seiha = TRUE;
			}
			else if (max_dlv[i] == d_info[i].maxdepth) seiha = TRUE;
			
			fprintf(fff, _("%c%-12s :  %3d 階\n", "%c%-16s :  level %3d\n"), seiha ? '!' : ' ', d_name + d_info[i].name, max_dlv[i]);
		}
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
	show_file(TRUE, file_name, _("今までに入ったダンジョン", "Dungeon"), 0, 0);
	
	/* Remove the file */
	fd_kill(file_name);
}

/*
* List virtues & status
*
*/
static void do_cmd_knowledge_stat(void)
{
	FILE *fff;
	
	char file_name[1024];
	int percent, v_nr;
	
	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}
	
	if (fff)
	{
		percent = (int)(((long)p_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
			(2 * p_ptr->hitdie +
			((PY_MAX_LEVEL - 1+3) * (p_ptr->hitdie + 1))));

#ifdef JP
		if (p_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "現在の体力ランク : %d/100\n\n", percent);
		else fprintf(fff, "現在の体力ランク : ???\n\n");
		fprintf(fff, "能力の最大値\n\n");
#else
		if (p_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "Your current Life Rating is %d/100.\n\n", percent);
		else fprintf(fff, "Your current Life Rating is ???.\n\n");
		fprintf(fff, "Limits of maximum stats\n\n");
#endif
		for (v_nr = 0; v_nr < 6; v_nr++)
		{
			if ((p_ptr->knowledge & KNOW_STAT) || p_ptr->stat_max[v_nr] == p_ptr->stat_max_max[v_nr]) fprintf(fff, "%s 18/%d\n", stat_names[v_nr], p_ptr->stat_max_max[v_nr]-18);
			else fprintf(fff, "%s ???\n", stat_names[v_nr]);
		}
	}

	dump_yourself(fff);

	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
	show_file(TRUE, file_name, _("自分に関する情報", "HP-rate & Max stat"), 0, 0);
	
	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Print all active quests
 */
static void do_cmd_knowledge_quests_current(FILE *fff)
{
	char tmp_str[120];
	char rand_tmp_str[120] = "\0";
	char name[80];
	monster_race *r_ptr;
	int i;
	int rand_level = 100;
	int total = 0;

	fprintf(fff, _("《遂行中のクエスト》\n", "< Current Quest >\n"));

	for (i = 1; i < max_quests; i++)
	{
		if ((quest[i].status == QUEST_STATUS_TAKEN) ||
			((quest[i].status == QUEST_STATUS_STAGE_COMPLETED) && (quest[i].type == QUEST_TYPE_TOWER)) ||
			(quest[i].status == QUEST_STATUS_COMPLETED))
		{
			/* Set the quest number temporary */
			int old_quest = p_ptr->inside_quest;
			int j;

			/* Clear the text */
			for (j = 0; j < 10; j++) quest_text[j][0] = '\0';
			quest_text_line = 0;

			p_ptr->inside_quest = i;

			/* Get the quest text */
			init_flags = INIT_SHOW_TEXT;

			process_dungeon_file("q_info.txt", 0, 0, 0, 0);

			/* Reset the old quest number */
			p_ptr->inside_quest = old_quest;

			/* No info from "silent" quests */
			if (quest[i].flags & QUEST_FLAG_SILENT) continue;

			total++;

			if (quest[i].type != QUEST_TYPE_RANDOM)
			{
				char note[80] = "\0";

				if (quest[i].status == QUEST_STATUS_TAKEN || quest[i].status == QUEST_STATUS_STAGE_COMPLETED)
				{
					switch (quest[i].type)
					{
					case QUEST_TYPE_KILL_LEVEL:
					case QUEST_TYPE_KILL_ANY_LEVEL:
						r_ptr = &r_info[quest[i].r_idx];
						strcpy(name, r_name + r_ptr->name);
						if (quest[i].max_num > 1)
						{
#ifdef JP
							sprintf(note," - %d 体の%sを倒す。(あと %d 体)",
								quest[i].max_num, name, quest[i].max_num - quest[i].cur_num);
#else
							plural_aux(name);
							sprintf(note," - kill %d %s, have killed %d.",
								quest[i].max_num, name, quest[i].cur_num);
#endif
						}
						else
							sprintf(note,_(" - %sを倒す。", " - kill %s."),name);
						break;

					case QUEST_TYPE_FIND_ARTIFACT:
						if (quest[i].k_idx)
						{
							artifact_type *a_ptr = &a_info[quest[i].k_idx];
							object_type forge;
							object_type *q_ptr = &forge;
							int k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
							object_prep(q_ptr, k_idx);
							q_ptr->name1 = quest[i].k_idx;
							q_ptr->ident = IDENT_STORE;
							object_desc(name, q_ptr, OD_NAME_ONLY);
						}
						sprintf(note,_("\n   - %sを見つけ出す。", "\n   - Find out %s."), name);
						break;
					case QUEST_TYPE_FIND_EXIT:
						sprintf(note,_(" - 出口に到達する。", " - Reach to Exit."));
						break;

					case QUEST_TYPE_KILL_NUMBER:
#ifdef JP
						sprintf(note," - %d 体のモンスターを倒す。(あと %d 体)",
							quest[i].max_num, quest[i].max_num - quest[i].cur_num);
#else
						sprintf(note," - Kill %d monsters, have killed %d.",
							quest[i].max_num, quest[i].cur_num);
#endif
						break;

					case QUEST_TYPE_KILL_ALL:
					case QUEST_TYPE_TOWER:
						sprintf(note,_(" - 全てのモンスターを倒す。", " - Kill all monsters."));
						break;
					}
				}

				/* Print the quest info */
#ifdef JP
				sprintf(tmp_str, "  %s (危険度:%d階相当)%s\n",
					quest[i].name, quest[i].level, note);
#else
				sprintf(tmp_str, "  %s (Danger level: %d)%s\n",
					quest[i].name, quest[i].level, note);
#endif

				fputs(tmp_str, fff);

				if (quest[i].status == QUEST_STATUS_COMPLETED)
				{
					sprintf(tmp_str, _("    クエスト達成 - まだ報酬を受けとってない。\n", "    Quest Completed - Unrewarded\n"));
					fputs(tmp_str, fff);
				}
				else
				{
					j = 0;

					while (quest_text[j][0] && j < 10)
					{
						fprintf(fff, "    %s\n", quest_text[j]);
						j++;
					}
				}
			}
			else if (quest[i].level < rand_level) /* QUEST_TYPE_RANDOM */
			{
				/* New random */
				rand_level = quest[i].level;

				if (max_dlv[DUNGEON_ANGBAND] >= rand_level)
				{
					/* Print the quest info */
					r_ptr = &r_info[quest[i].r_idx];
					strcpy(name, r_name + r_ptr->name);

					if (quest[i].max_num > 1)
					{
#ifdef JP
						sprintf(rand_tmp_str,"  %s (%d 階) - %d 体の%sを倒す。(あと %d 体)\n",
							quest[i].name, quest[i].level,
							quest[i].max_num, name, quest[i].max_num - quest[i].cur_num);
#else
						plural_aux(name);

						sprintf(rand_tmp_str,"  %s (Dungeon level: %d)\n  Kill %d %s, have killed %d.\n",
							quest[i].name, quest[i].level,
							quest[i].max_num, name, quest[i].cur_num);
#endif
					}
					else
					{
#ifdef JP
						sprintf(rand_tmp_str,"  %s (%d 階) - %sを倒す。\n",
							quest[i].name, quest[i].level, name);
#else
						sprintf(rand_tmp_str,"  %s (Dungeon level: %d)\n  Kill %s.\n",
							quest[i].name, quest[i].level, name);
#endif
					}
				}
			}
		}
	}

	/* Print the current random quest  */
	if (rand_tmp_str[0]) fputs(rand_tmp_str, fff);

	if (!total) fprintf(fff, _("  なし\n", "  Nothing.\n"));
}


static bool do_cmd_knowledge_quests_aux(FILE *fff, int q_idx)
{
	char tmp_str[120];
	char playtime_str[16];
	quest_type* const q_ptr = &quest[q_idx];

	if (is_fixed_quest_idx(q_idx))
	{
		/* Set the quest number temporary */
		int old_quest = p_ptr->inside_quest;

		p_ptr->inside_quest = q_idx;

		/* Get the quest */
		init_flags = INIT_NAME_ONLY;

		process_dungeon_file("q_info.txt", 0, 0, 0, 0);

		/* Reset the old quest number */
		p_ptr->inside_quest = old_quest;

		/* No info from "silent" quests */
		if (q_ptr->flags & QUEST_FLAG_SILENT) return FALSE;
	}

	strnfmt(playtime_str, sizeof(playtime_str), "%02d:%02d:%02d",
		q_ptr->comptime/(60*60), (q_ptr->comptime/60)%60, q_ptr->comptime%60);

	if (!is_fixed_quest_idx(q_idx) && q_ptr->r_idx)
	{
		/* Print the quest info */
		if (q_ptr->complev == 0)
		{
			sprintf(tmp_str,
				_("  %-35s (%3d階)            -   不戦勝 - %s\n",
				  "  %-35s (Dungeon level: %3d) - Unearned - %s\n") ,
				r_name+r_info[q_ptr->r_idx].name,
				q_ptr->level, playtime_str);
		}
		else
		{
			sprintf(tmp_str,
				_("  %-35s (%3d階)            - レベル%2d - %s\n",
				  "  %-35s (Dungeon level: %3d) - level %2d - %s\n") ,
				r_name+r_info[q_ptr->r_idx].name,
				q_ptr->level,
				q_ptr->complev,
				playtime_str);
		}
	}
	else
	{
		/* Print the quest info */
		sprintf(tmp_str,
			_("  %-35s (危険度:%3d階相当) - レベル%2d - %s\n",
			  "  %-35s (Danger  level: %3d) - level %2d - %s\n") ,
			q_ptr->name, q_ptr->level, q_ptr->complev, playtime_str);
	}

	fputs(tmp_str, fff);

	return TRUE;
}

/*
 * Print all finished quests
 */
void do_cmd_knowledge_quests_completed(FILE *fff, int quest_num[])
{
	int i;
	int total = 0;

	fprintf(fff, _("《達成したクエスト》\n", "< Completed Quest >\n"));
	for (i = 1; i < max_quests; i++)
	{
		int q_idx = quest_num[i];
		quest_type* const q_ptr = &quest[q_idx];

		if (q_ptr->status == QUEST_STATUS_FINISHED &&
		    do_cmd_knowledge_quests_aux(fff, q_idx))
		{
			++ total;
		}
	}
	if (!total) fprintf(fff, _("  なし\n", "  Nothing.\n"));
}


/*
 * Print all failed quests
 */
void do_cmd_knowledge_quests_failed(FILE *fff, int quest_num[])
{
	int i;
	int total = 0;

	fprintf(fff, _("《失敗したクエスト》\n", "< Failed Quest >\n"));
	for (i = 1; i < max_quests; i++)
	{
		int q_idx = quest_num[i];
		quest_type* const q_ptr = &quest[q_idx];

		if (((q_ptr->status == QUEST_STATUS_FAILED_DONE) || (q_ptr->status == QUEST_STATUS_FAILED)) &&
		    do_cmd_knowledge_quests_aux(fff, q_idx))
		{
			++ total;
		}
	}
	if (!total) fprintf(fff, _("  なし\n", "  Nothing.\n"));
}


/*
 * Print all random quests
 */
static void do_cmd_knowledge_quests_wiz_random(FILE *fff)
{
	char tmp_str[120];
	int i;
	int total = 0;

	fprintf(fff, _("《残りのランダムクエスト》\n", "< Remaining Random Quest >\n"));
	for (i = 1; i < max_quests; i++)
	{
		/* No info from "silent" quests */
		if (quest[i].flags & QUEST_FLAG_SILENT) continue;

		if ((quest[i].type == QUEST_TYPE_RANDOM) && (quest[i].status == QUEST_STATUS_TAKEN))
		{
			total++;

			/* Print the quest info */
#ifdef JP
			sprintf(tmp_str, "  %s (%d階, %s)\n",
				quest[i].name, quest[i].level, r_name+r_info[quest[i].r_idx].name);
#else
			sprintf(tmp_str, "  %s (%d, %s)\n",
				quest[i].name, quest[i].level, r_name+r_info[quest[i].r_idx].name);
#endif
			fputs(tmp_str, fff);
		}
	}
	if (!total) fprintf(fff, _("  なし\n", "  Nothing.\n"));
}


bool ang_sort_comp_quest_num(vptr u, vptr v, int a, int b)
{
	int *q_num = (int *)u;
	quest_type *qa = &quest[q_num[a]];
	quest_type *qb = &quest[q_num[b]];

	/* Unused */
	(void)v;

	return (qa->comptime <= qb->comptime);
}

void ang_sort_swap_quest_num(vptr u, vptr v, int a, int b)
{
	int *q_num = (int *)u;
	int tmp;

	/* Unused */
	(void)v;

	tmp = q_num[a];
	q_num[a] = q_num[b];
	q_num[b] = tmp;
}


/*
 * Print quest status of all active quests
 */
static void do_cmd_knowledge_quests(void)
{
	FILE *fff;
	char file_name[1024];
	int *quest_num, dummy, i;

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff)
	{
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	/* Allocate Memory */
	C_MAKE(quest_num, max_quests, int);

	/* Sort by compete level */
	for (i = 1; i < max_quests; i++) quest_num[i] = i;
	ang_sort_comp = ang_sort_comp_quest_num;
	ang_sort_swap = ang_sort_swap_quest_num;
	ang_sort(quest_num, &dummy, max_quests);

	/* Dump Quest Information */
	do_cmd_knowledge_quests_current(fff);
	fputc('\n', fff);
	do_cmd_knowledge_quests_completed(fff, quest_num);
	fputc('\n', fff);
	do_cmd_knowledge_quests_failed(fff, quest_num);
	if (p_ptr->wizard)
	{
		fputc('\n', fff);
		do_cmd_knowledge_quests_wiz_random(fff);
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("クエスト達成状況", "Quest status"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);

	/* Free Memory */
	C_KILL(quest_num, max_quests, int);
}


/*
 * List my home
 */
static void do_cmd_knowledge_home(void)
{
	FILE *fff;

	int i;
	char file_name[1024];
	store_type  *st_ptr;
	char o_name[MAX_NLEN];
	cptr		paren = ")";

	process_dungeon_file("w_info.txt", 0, 0, max_wild_y, max_wild_x);

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
		msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
		msg_print(NULL);
		return;
	}

	if (fff)
	{
		/* Print all homes in the different towns */
		st_ptr = &town[1].store[STORE_HOME];

		/* Home -- if anything there */
		if (st_ptr->stock_num)
		{
#ifdef JP
			int x = 1;
#endif
			/* Header with name of the town */
			fprintf(fff, _("  [ 我が家のアイテム ]\n", "  [Home Inventory]\n"));

			/* Dump all available items */
			for (i = 0; i < st_ptr->stock_num; i++)
			{
#ifdef JP
				if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
				object_desc(o_name, &st_ptr->stock[i], 0);
				if (strlen(o_name) <= 80-3)
				{
					fprintf(fff, "%c%s %s\n", I2A(i%12), paren, o_name);
				}
				else
				{
					int n;
					char *t;
					for (n = 0, t = o_name; n < 80-3; n++, t++)
						if(iskanji(*t)) {t++; n++;}
					if (n == 81-3) n = 79-3; /* 最後が漢字半分 */

					fprintf(fff, "%c%s %.*s\n", I2A(i%12), paren, n, o_name);
					fprintf(fff, "   %.77s\n", o_name+n);
				}
#else
				object_desc(o_name, &st_ptr->stock[i], 0);
				fprintf(fff, "%c%s %s\n", I2A(i%12), paren, o_name);
#endif

			}

			/* Add an empty line */
			fprintf(fff, "\n\n");
		}
	}

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
	show_file(TRUE, file_name, _("我が家のアイテム", "Home Inventory"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Check the status of "autopick"
 */
static void do_cmd_knowledge_autopick(void)
{
	int k;
	FILE *fff;
	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	if (!fff)
	{
	    msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
	    msg_print(NULL);
	    return;
	}

	if (!max_autopick)
	{
	    fprintf(fff, _("自動破壊/拾いには何も登録されていません。", "No preference for auto picker/destroyer."));
	}
	else
	{
		fprintf(fff, _("   自動拾い/破壊には現在 %d行登録されています。\n\n",
					   "   There are %d registered lines for auto picker/destroyer.\n\n"), max_autopick);
	}

	for (k = 0; k < max_autopick; k++)
	{
		cptr tmp;
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
		else /* if (act & DO_QUERY_AUTOPICK) */ /* Obvious */
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
	/* Close the file */
	my_fclose(fff);
	/* Display the file contents */
	show_file(TRUE, file_name, _("自動拾い/破壊 設定リスト", "Auto-picker/Destroyer"), 0, 0);

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Interact with "knowledge"
 */
void do_cmd_knowledge(void)
{
	int i, p = 0;
	bool need_redraw = FALSE;

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save the screen */
	screen_save();

	/* Interact until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Ask for a choice */
#ifdef JP
		prt(format("%d/2 ページ", (p+1)), 2, 65);
		prt("現在の知識を確認する", 3, 0);
#else
		prt(format("page %d/2", (p+1)), 2, 65);
		prt("Display current knowledge", 3, 0);
#endif

		/* Give some choices */
#ifdef JP
		if (p == 0)
		{
			prt("(1) 既知の伝説のアイテム                 の一覧", 6, 5);
			prt("(2) 既知のアイテム                       の一覧", 7, 5);
			prt("(3) 既知の生きているユニーク・モンスター の一覧", 8, 5);
			prt("(4) 既知のモンスター                     の一覧", 9, 5);
			prt("(5) 倒した敵の数                         の一覧", 10, 5);
			if (!vanilla_town) prt("(6) 賞金首                               の一覧", 11, 5);
			prt("(7) 現在のペット                         の一覧", 12, 5);
			prt("(8) 我が家のアイテム                     の一覧", 13, 5);
			prt("(9) *鑑定*済み装備の耐性                 の一覧", 14, 5);
			prt("(0) 地形の表示文字/タイル                の一覧", 15, 5);
		}
		else
		{
			prt("(a) 自分に関する情報                     の一覧", 6, 5);
			prt("(b) 突然変異                             の一覧", 7, 5);
			prt("(c) 武器の経験値                         の一覧", 8, 5);
			prt("(d) 魔法の経験値                         の一覧", 9, 5);
			prt("(e) 技能の経験値                         の一覧", 10, 5);
			prt("(f) プレイヤーの徳                       の一覧", 11, 5);
			prt("(g) 入ったダンジョン                     の一覧", 12, 5);
			prt("(h) 実行中のクエスト                     の一覧", 13, 5);
			prt("(i) 現在の自動拾い/破壊設定              の一覧", 14, 5);
		}
#else
		if (p == 0)
		{
			prt("(1) Display known artifacts", 6, 5);
			prt("(2) Display known objects", 7, 5);
			prt("(3) Display remaining uniques", 8, 5);
			prt("(4) Display known monster", 9, 5);
			prt("(5) Display kill count", 10, 5);
			if (!vanilla_town) prt("(6) Display wanted monsters", 11, 5);
			prt("(7) Display current pets", 12, 5);
			prt("(8) Display home inventory", 13, 5);
			prt("(9) Display *identified* equip.", 14, 5);
			prt("(0) Display terrain symbols.", 15, 5);
		}
		else
		{
			prt("(a) Display about yourself", 6, 5);
			prt("(b) Display mutations", 7, 5);
			prt("(c) Display weapon proficiency", 8, 5);
			prt("(d) Display spell proficiency", 9, 5);
			prt("(e) Display misc. proficiency", 10, 5);
			prt("(f) Display virtues", 11, 5);
			prt("(g) Display dungeons", 12, 5);
			prt("(h) Display current quests", 13, 5);
			prt("(i) Display auto pick/destroy", 14, 5);
		}
#endif
		/* Prompt */
#ifdef JP
		prt("-続く-", 17, 8);
		prt("ESC) 抜ける", 21, 1);
		prt("SPACE) 次ページ", 21, 30);
		/*prt("-) 前ページ", 21, 60);*/
		prt("コマンド:", 20, 0);
#else
		prt("-more-", 17, 8);
		prt("ESC) Exit menu", 21, 1);
		prt("SPACE) Next page", 21, 30);
		/*prt("-) Previous page", 21, 60);*/
		prt("Command: ", 20, 0);
#endif

		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;
		switch (i)
		{
		case ' ': /* Page change */
		case '-':
			p = 1 - p;
			break;
		case '1': /* Artifacts */
			do_cmd_knowledge_artifacts();
			break;
		case '2': /* Objects */
			do_cmd_knowledge_objects(&need_redraw, FALSE, -1);
			break;
		case '3': /* Uniques */
			do_cmd_knowledge_uniques();
			break;
		case '4': /* Monsters */
			do_cmd_knowledge_monsters(&need_redraw, FALSE, -1);
			break;
		case '5': /* Kill count  */
			do_cmd_knowledge_kill_count();
			break;
		case '6': /* wanted */
			if (!vanilla_town) do_cmd_knowledge_kubi();
			break;
		case '7': /* Pets */
			do_cmd_knowledge_pets();
			break;
		case '8': /* Home */
			do_cmd_knowledge_home();
			break;
		case '9': /* Resist list */
			do_cmd_knowledge_inven();
			break;
		case '0': /* Feature list */
			{
				int lighting_level = F_LIT_STANDARD;
				do_cmd_knowledge_features(&need_redraw, FALSE, -1, &lighting_level);
			}
			break;
		/* Next page */
		case 'a': /* Max stat */
			do_cmd_knowledge_stat();
			break;
		case 'b': /* Mutations */
			do_cmd_knowledge_mutations();
			break;
		case 'c': /* weapon-exp */
			do_cmd_knowledge_weapon_exp();
			break;
		case 'd': /* spell-exp */
			do_cmd_knowledge_spell_exp();
			break;
		case 'e': /* skill-exp */
			do_cmd_knowledge_skill_exp();
			break;
		case 'f': /* Virtues */
			do_cmd_knowledge_virtues();
			break;
		case 'g': /* Dungeon */
			do_cmd_knowledge_dungeon();
			break;
		case 'h': /* Quests */
			do_cmd_knowledge_quests();
			break;
		case 'i': /* Autopick */
			do_cmd_knowledge_autopick();
			break;
		default: /* Unknown option */
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Restore the screen */
	screen_load();

	if (need_redraw) do_cmd_redraw();
}


/*
 * Check on the status of an active quest
 */
void do_cmd_checkquest(void)
{
	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Save the screen */
	screen_save();

	/* Quest info */
	do_cmd_knowledge_quests();

	/* Restore the screen */
	screen_load();
}


/*
 * Display the time and date
 */
void do_cmd_time(void)
{
	int day, hour, min, full, start, end, num;
	char desc[1024];

	char buf[1024];
	char day_buf[10];

	FILE *fff;

	extract_day_hour_min(&day, &hour, &min);

	full = hour * 100 + min;

	start = 9999;
	end = -9999;

	num = 0;

	strcpy(desc, _("変な時刻だ。", "It is a strange time."));

	if (day < MAX_DAYS) sprintf(day_buf, "%d", day);
	else strcpy(day_buf, "*****");

	/* Message */
#ifdef JP
	msg_format("%s日目, 時刻は%d:%02d %sです。",
		   day_buf, (hour % 12 == 0) ? 12 : (hour % 12),
		   min, (hour < 12) ? "AM" : "PM");
#else
	msg_format("This is day %s. The time is %d:%02d %s.",
		   day_buf, (hour % 12 == 0) ? 12 : (hour % 12),
		   min, (hour < 12) ? "AM" : "PM");
#endif


	/* Find the path */
	if (!randint0(10) || p_ptr->image)
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timefun_j.txt", "timefun.txt"));
	}
	else
	{
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("timenorm_j.txt", "timenorm.txt"));
	}

	/* Open this file */
	fff = my_fopen(buf, "rt");

	/* Oops */
	if (!fff) return;

	/* Find this time */
	while (!my_fgets(fff, buf, sizeof(buf)))
	{
		/* Ignore comments */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Ignore invalid lines */
		if (buf[1] != ':') continue;

		/* Process 'Start' */
		if (buf[0] == 'S')
		{
			/* Extract the starting time */
			start = atoi(buf + 2);

			/* Assume valid for an hour */
			end = start + 59;

			/* Next... */
			continue;
		}

		/* Process 'End' */
		if (buf[0] == 'E')
		{
			/* Extract the ending time */
			end = atoi(buf + 2);

			/* Next... */
			continue;
		}

		/* Ignore incorrect range */
		if ((start > full) || (full > end)) continue;

		/* Process 'Description' */
		if (buf[0] == 'D')
		{
			num++;

			/* Apply the randomizer */
			if (!randint0(num)) strcpy(desc, buf + 2);

			/* Next... */
			continue;
		}
	}

	/* Message */
	msg_print(desc);

	/* Close the file */
	my_fclose(fff);
}
