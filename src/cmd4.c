/* File: cmd4.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Interface commands */

#include "angband.h"



/*
 * A set of functions to maintain automatic dumps of various kinds.
 * -Mogami-
 *
 * remove_auto_dump(orig_file, mark)
 *     Remove the old automatic dump of type "mark".
 * auto_dump_printf(fmt, ...)
 *     Dump a formatted string using fprintf().
 * open_auto_dump(buf, mark)
 *     Open a file, remove old dump, and add new header.
 * close_auto_dump(void)
 *     Add a footer, and close the file.
 *
 *    The dump commands of original Angband simply add new lines to
 * existing files; these files will become bigger and bigger unless
 * an user deletes some or all of these files by hand at some
 * point.
 *
 *     These three functions automatically delete old dumped lines 
 * before adding new ones.  Since there are various kinds of automatic 
 * dumps in a single file, we add a header and a footer with a type 
 * name for every automatic dump, and kill old lines only when the 
 * lines have the correct type of header and footer.
 *
 *     We need to be quite paranoid about correctness; the user might 
 * (mistakenly) edit the file by hand, and see all their work come
 * to nothing on the next auto dump otherwise.  The current code only 
 * detects changes by noting inconsistencies between the actual number 
 * of lines and the number written in the footer.  Note that this will 
 * not catch single-line edits.
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

/*
 * Remove old lines automatically generated before.
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", tmp_file);
#else
	    msg_format("Failed to create temporary file %s.", tmp_file);
#endif
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


/*
 * Dump a formatted line, using "vstrnfmt()".
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


/*
 *  Open file to append auto dump.
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
#ifdef JP
		msg_format("%s を開くことができませんでした。", buf);
#else
		msg_format("Failed to open %s.", buf);
#endif
		msg_print(NULL);

		/* Failed */
		return FALSE;
	}

	/* Start dumping */
	fprintf(auto_dump_stream, "%s\n", header_mark_str);

	/* Initialize counter */
	auto_dump_line_num = 0;

#ifdef JP
	auto_dump_printf("# *警告!!* 以降の行は自動生成されたものです。\n");
	auto_dump_printf("# *警告!!* 後で自動的に削除されるので編集しないでください。\n");
#else
	auto_dump_printf("# *Warning!*  The lines below are an automatic dump.\n");
	auto_dump_printf("# Don't edit them; changes will be deleted and replaced automatically.\n");
#endif

	/* Success */
	return TRUE;
}

/*
 *  Append foot part and close auto dump.
 */
static void close_auto_dump(void)
{
	char footer_mark_str[80];

	/* Prepare a footer mark string */
	sprintf(footer_mark_str, auto_dump_footer, auto_dump_mark);

#ifdef JP
	auto_dump_printf("# *警告!!* 以上の行は自動生成されたものです。\n");
	auto_dump_printf("# *警告!!* 後で自動的に削除されるので編集しないでください。\n");
#else
	auto_dump_printf("# *Warning!*  The lines above are an automatic dump.\n");
	auto_dump_printf("# Don't edit them; changes will be deleted and replaced automatically.\n");
#endif

	/* End of dump */
	fprintf(auto_dump_stream, "%s (%d)\n", footer_mark_str, auto_dump_line_num);

	/* Close */
	my_fclose(auto_dump_stream);

	return;
}


#ifndef JP
/*
 * Return suffix of ordinal number
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


/*
 *   Take note to the diary.
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
		init_flags = INIT_ASSIGN;

		process_dungeon_file("q_info.txt", 0, 0, 0, 0);

		/* Reset the old quest number */
		p_ptr->inside_quest = old_quest;
	}

#ifdef JP
	sprintf(file_name,"playrecord-%s.txt",savefile_base);
#else
	/* different filne name to avoid mixing */
	sprintf(file_name,"playrec-%s.txt",savefile_base);
#endif

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	fff = my_fopen(buf, "a");

	/* Failure */
	if (!fff)
	{
#ifdef JP
		msg_format("%s を開くことができませんでした。プレイ記録を一時停止します。", buf);
#else
		msg_format("Failed to open %s. Play-Record is disabled temporally.", buf);
#endif
		msg_format(NULL);
		disable_nikki=TRUE;
		return (-1);
	}

	q_idx = quest_number(dun_level);

	if (write_level)
	{
		if (p_ptr->inside_arena)
#ifdef JP
			note_level = "アリーナ:";
#else
			note_level = "Arane:";
#endif
		else if (!dun_level)
#ifdef JP
			note_level = "地上:";
#else
			note_level = "Surface:";
#endif
		else if (q_idx && (is_fixed_quest_idx(q_idx)
		         && !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
#ifdef JP
			note_level = "クエスト:";
#else
			note_level = "Quest:";
#endif
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
#ifdef JP
			fprintf(fff, "%d日目\n",day);
#else
			fprintf(fff, "Day %d\n",day);
#endif
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
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sを発見した。\n", hour, min, note_level, note);
#else
			fprintf(fff, " %2d:%02d %20s discovered %s.\n", hour, min, note_level, note);
#endif
			break;
		}
		case NIKKI_UNIQUE:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sを倒した。\n", hour, min, note_level, note);
#else
			fprintf(fff, " %2d:%02d %20s defeated %s.\n", hour, min, note_level, note);
#endif
			break;
		}
		case NIKKI_FIX_QUEST_C:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
#ifdef JP
			fprintf(fff, " %2d:%02d %20s クエスト「%s」を達成した。\n", hour, min, note_level, quest[num].name);
#else
			fprintf(fff, " %2d:%02d %20s completed quest '%s'.\n", hour, min, note_level, quest[num].name);
#endif
			break;
		}
		case NIKKI_FIX_QUEST_F:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
#ifdef JP
			fprintf(fff, " %2d:%02d %20s クエスト「%s」から命からがら逃げ帰った。\n", hour, min, note_level, quest[num].name);
#else
			fprintf(fff, " %2d:%02d %20s run away from quest '%s'.\n", hour, min, note_level, quest[num].name);
#endif
			break;
		}
		case NIKKI_RAND_QUEST_C:
		{
			char name[80];
			strcpy(name, r_name+r_info[quest[num].r_idx].name);
#ifdef JP
			fprintf(fff, " %2d:%02d %20s ランダムクエスト(%s)を達成した。\n", hour, min, note_level, name);
#else
			fprintf(fff, " %2d:%02d %20s completed randome quest '%s'\n", hour, min, note_level, name);
#endif
			break;
		}
		case NIKKI_RAND_QUEST_F:
		{
			char name[80];
			strcpy(name, r_name+r_info[quest[num].r_idx].name);
#ifdef JP
			fprintf(fff, " %2d:%02d %20s ランダムクエスト(%s)から逃げ出した。\n", hour, min, note_level, name);
#else
			fprintf(fff, " %2d:%02d %20s ran away from quest '%s'.\n", hour, min, note_level, name);
#endif
			break;
		}
		case NIKKI_MAXDEAPTH:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sの最深階%d階に到達した。\n", hour, min, note_level, d_name+d_info[dungeon_type].name, num);
#else
			fprintf(fff, " %2d:%02d %20s reached level %d of %s for the first time.\n", hour, min, note_level, num, d_name+d_info[dungeon_type].name);
#endif
			break;
		}
		case NIKKI_TRUMP:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %s%sの最深階を%d階にセットした。\n", hour, min, note_level, note, d_name + d_info[num].name, max_dlv[num]);
#else
			fprintf(fff, " %2d:%02d %20s reset recall level of %s to %d %s.\n", hour, min, note_level, d_name + d_info[num].name, max_dlv[num], note);
#endif
			break;
		}
		case NIKKI_STAIR:
		{
			cptr to;
			if (q_idx && (is_fixed_quest_idx(q_idx)
			     && !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
			{
#ifdef JP
				to = "地上";
#else
				to = "the surface";
#endif
			}
			else
			{
#ifdef JP
				if (!(dun_level+num)) to = "地上";
				else to = format("%d階", dun_level+num);
#else
				if (!(dun_level+num)) to = "the surface";
				else to = format("level %d", dun_level+num);
#endif
			}

#ifdef JP 
			fprintf(fff, " %2d:%02d %20s %sへ%s。\n", hour, min, note_level, to, note);
#else
			fprintf(fff, " %2d:%02d %20s %s %s.\n", hour, min, note_level, note, to);
#endif
			break;
		}
		case NIKKI_RECALL:
		{
			if (!num)
#ifdef JP
				fprintf(fff, " %2d:%02d %20s 帰還を使って%sの%d階へ下りた。\n", hour, min, note_level, d_name+d_info[dungeon_type].name, max_dlv[dungeon_type]);
#else
				fprintf(fff, " %2d:%02d %20s recalled to dungeon level %d of %s.\n", hour, min, note_level, max_dlv[dungeon_type], d_name+d_info[dungeon_type].name);
#endif
			else
#ifdef JP
				fprintf(fff, " %2d:%02d %20s 帰還を使って地上へと戻った。\n", hour, min, note_level);
#else
				fprintf(fff, " %2d:%02d %20s recalled from dungeon to surface.\n", hour, min, note_level);
#endif
			break;
		}
		case NIKKI_TO_QUEST:
		{
			if (quest[num].flags & QUEST_FLAG_SILENT) break;
#ifdef JP
			fprintf(fff, " %2d:%02d %20s クエスト「%s」へと突入した。\n", hour, min, note_level, quest[num].name);
#else
			fprintf(fff, " %2d:%02d %20s entered the quest '%s'.\n", hour, min, note_level, quest[num].name);
#endif
			break;
		}
		case NIKKI_TELE_LEV:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s レベル・テレポートで脱出した。\n", hour, min, note_level);
#else
			fprintf(fff, " %2d:%02d %20s Got out using teleport level.\n", hour, min, note_level);
#endif
			break;
		}
		case NIKKI_BUY:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sを購入した。\n", hour, min, note_level, note);
#else
			fprintf(fff, " %2d:%02d %20s bought %s.\n", hour, min, note_level, note);
#endif
			break;
		}
		case NIKKI_SELL:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sを売却した。\n", hour, min, note_level, note);
#else
			fprintf(fff, " %2d:%02d %20s sold %s.\n", hour, min, note_level, note);
#endif
			break;
		}
		case NIKKI_ARENA:
		{
			if (num < 0)
			{
#ifdef JP
				fprintf(fff, " %2d:%02d %20s 闘技場の%d回戦で、%sの前に敗れ去った。\n", hour, min, note_level, -num, note);
#else
				int n = -num;
				fprintf(fff, " %2d:%02d %20s beaten by %s in the %d%s fight.\n", hour, min, note_level, note, n, get_ordinal_number_suffix(n));
#endif
				break;
			}
#ifdef JP
			fprintf(fff, " %2d:%02d %20s 闘技場の%d回戦(%s)に勝利した。\n", hour, min, note_level, num, note);
#else
			fprintf(fff, " %2d:%02d %20s won the %d%s fight (%s).\n", hour, min, note_level, num, get_ordinal_number_suffix(num), note);
#endif
			if (num == MAX_ARENA_MONS)
			{
#ifdef JP
				fprintf(fff, "                 闘技場のすべての敵に勝利し、チャンピオンとなった。\n");
#else
				fprintf(fff, "                 won all fight to become a Chanpion.\n");
#endif
				do_level = FALSE;
			}
			break;
		}
		case NIKKI_HANMEI:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sを識別した。\n", hour, min, note_level, note);
#else
			fprintf(fff, " %2d:%02d %20s identified %s.\n", hour, min, note_level, note);
#endif
			break;
		}
		case NIKKI_WIZ_TELE:
		{
			cptr to;
			if (!dun_level)
#ifdef JP
				to = "地上";
#else
				to = "the surface";
#endif
			else
#ifdef JP
				to = format("%d階(%s)", dun_level, d_name+d_info[dungeon_type].name);
#else
				to = format("level %d of %s", dun_level, d_name+d_info[dungeon_type].name);
#endif

#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sへとウィザード・テレポートで移動した。\n", hour, min, note_level, to);
#else
			fprintf(fff, " %2d:%02d %20s wizard-teleport to %s.\n", hour, min, note_level, to);
#endif
			break;
		}
		case NIKKI_PAT_TELE:
		{
			cptr to;
			if (!dun_level)
#ifdef JP
				to = "地上";
#else
				to = "the surface";
#endif
			else
#ifdef JP
				to = format("%d階(%s)", dun_level, d_name+d_info[dungeon_type].name);
#else
				to = format("level %d of %s", dun_level, d_name+d_info[dungeon_type].name);
#endif

#ifdef JP
			fprintf(fff, " %2d:%02d %20s %sへとパターンの力で移動した。\n", hour, min, note_level, to);
#else
			fprintf(fff, " %2d:%02d %20s used Pattern to teleport to %s.\n", hour, min, note_level, to);
#endif
			break;
		}
		case NIKKI_LEVELUP:
		{
#ifdef JP
			fprintf(fff, " %2d:%02d %20s レベルが%dに上がった。\n", hour, min, note_level, num);
#else
			fprintf(fff, " %2d:%02d %20s reached player level %d.\n", hour, min, note_level, num);
#endif
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
				case 0:
#ifdef JP
					fprintf(fff, "%sを旅の友にすることに決めた。\n", note);
#else
					fprintf(fff, "decided to travel together with %s.\n", note);
#endif
					break;
				case 1:
#ifdef JP
					fprintf(fff, "%sの名前を消した。\n", note);
#else
					fprintf(fff, "unnamed %s.\n", note);
#endif
					break;
				case 2:
#ifdef JP
					fprintf(fff, "%sを解放した。\n", note);
#else
					fprintf(fff, "dismissed %s.\n", note);
#endif
					break;
				case 3:
#ifdef JP
					fprintf(fff, "%sが死んでしまった。\n", note);
#else
					fprintf(fff, "%s died.\n", note);
#endif
					break;
				case 4:
#ifdef JP
					fprintf(fff, "%sをおいて別のマップへ移動した。\n", note);
#else
					fprintf(fff, "moved to another map leaving %s behind.\n", note);
#endif
					break;
				case 5:
#ifdef JP
					fprintf(fff, "%sとはぐれてしまった。\n", note);
#else
					fprintf(fff, "lost sight of %s.\n", note);
#endif
					break;
				case 6:
#ifdef JP
					fprintf(fff, "%sが*破壊*によって消え去った。\n", note);
#else
					fprintf(fff, "%s was made disappeared by *destruction*.\n", note);
#endif
					break;
				case 7:
#ifdef JP
					fprintf(fff, "%sが岩石に押し潰された。\n", note);
#else
					fprintf(fff, "%s was crushed by falling rocks.\n", note);
#endif
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
					   "ん〜？聞こえんなぁ",
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
#ifdef JP
	sprintf(file_name,"playrecord-%s.txt",savefile_base);
#else
	sprintf(file_name,"playrec-%s.txt",savefile_base);
#endif

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

static void do_cmd_bunshou(void)
{
	char tmp[80] = "\0";
	char bunshou[80] = "\0";

#ifdef JP
	if (get_string("内容: ", tmp, 79))
#else
	if (get_string("diary note: ", tmp, 79))
#endif
	{
		strcpy(bunshou, tmp);

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, bunshou);
	}
}

static void do_cmd_last_get(void)
{
	char buf[256];
	s32b turn_tmp;

	if (record_o_name[0] == '\0') return;

#ifdef JP
	sprintf(buf,"%sの入手を記録します。",record_o_name);
#else
	sprintf(buf,"Do you really want to record getting %s? ",record_o_name);
#endif
	if (!get_check(buf)) return;

	turn_tmp = turn;
	turn = record_turn;
#ifdef JP
	sprintf(buf,"%sを手に入れた。", record_o_name);
#else
	sprintf(buf,"descover %s.", record_o_name);
#endif
	do_cmd_write_nikki(NIKKI_BUNSHOU, 0, buf);
	turn = turn_tmp;
}

static void do_cmd_erase_nikki(void)
{
	char file_name[80];
	char buf[256];
	FILE *fff = NULL;

#ifdef JP
	if (!get_check("本当に記録を消去しますか？")) return;
#else
	if (!get_check("Do you really want to delete all your record? ")) return;
#endif

#ifdef JP
	sprintf(file_name,"playrecord-%s.txt",savefile_base);
#else
	sprintf(file_name,"playrec-%s.txt",savefile_base);
#endif

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, file_name);

	/* Remove the file */
	fd_kill(buf);

	fff = my_fopen(buf, "w");
	if(fff){
		my_fclose(fff);
#ifdef JP
		msg_format("記録を消去しました。");
#else
		msg_format("deleted record.");
#endif
	}else{
#ifdef JP
		msg_format("%s の消去に失敗しました。", buf);
#else
		msg_format("failed to delete %s.", buf);
#endif
	}
	msg_print(NULL);
}


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
#ifdef JP
		prt("[ 記録の設定 ]", 2, 0);
#else
		prt("[ Play Record ]", 2, 0);
#endif


		/* Give some choices */
#ifdef JP
		prt("(1) 記録を見る", 4, 5);
		prt("(2) 文章を記録する", 5, 5);
		prt("(3) 直前に入手又は鑑定したものを記録する", 6, 5);
		prt("(4) 記録を消去する", 7, 5);
#else
		prt("(1) Display your record", 4, 5);
		prt("(2) Add record", 5, 5);
		prt("(3) Record item you last get/identify", 6, 5);
		prt("(4) Delete your record", 7, 5);
#endif


		/* Prompt */
#ifdef JP
		prt("コマンド:", 18, 0);
#else
		prt("Command: ", 18, 0);
#endif


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
		default: /* Unknown option */
			bell();
		}

		/* Flush messages */
		msg_print(NULL);
	}

	/* Restore the screen */
	screen_load();
}

/*
 * Hack -- redraw the screen
 *
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This command is also used to "instantiate" the results of the user
 * selecting various things, such as graphics mode, so it must call
 * the "TERM_XTRA_REACT" hook before redrawing the windows.
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


/*
 * Hack -- change name
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
#ifdef JP
			if (get_string("ファイル名: ", tmp, 80))
#else
			if (get_string("File name: ", tmp, 80))
#endif

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


/*
 * Recall the most recent message
 */
void do_cmd_message_one(void)
{
	/* Recall one message XXX XXX XXX */
	prt(format("> %s", message_str(0)), 0, 0);
}


/*
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
 */
void do_cmd_messages(int num_now)
{
	int i, j, k, n;
	uint q;

	char shower[80];
	char finder[80];
	int wid, hgt;


	/* Get size */
	Term_get_size(&wid, &hgt);

	/* Wipe finder */
	strcpy(finder, "");

	/* Wipe shower */
	strcpy(shower, "");


	/* Total messages */
	n = message_num();

	/* Start on first message */
	i = 0;

	/* Start at leftmost edge */
	q = 0;

	/* Save the screen */
	screen_save();

	/* Process requests until done */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Dump up to 20 lines of messages */
		for (j = 0; (j < hgt - 4) && (i + j < n); j++)
		{
			cptr msg = message_str(i+j);

			/* Apply horizontal scroll */
			msg = (strlen(msg) >= q) ? (msg + q) : "";

			/* Dump the messages, bottom to top */
			Term_putstr(0, hgt-j-3, -1, (bool)(i+j < num_now ? TERM_WHITE : TERM_SLATE), msg);

			/* Hilite "shower" */
			if (shower[0])
			{
				cptr str = msg;

				/* Display matches */
				while ((str = strstr(str, shower)) != NULL)
				{
					int len = strlen(shower);

					/* Display the match */
					Term_putstr(str-msg, hgt-j-3, len, TERM_YELLOW, shower);

					/* Advance */
					str += len;
				}
			}
		}

		/* Display header XXX XXX XXX */
#ifdef JP
		/* translation */
		prt(format("以前のメッセージ %d-%d 全部で(%d) オフセット(%d)",
			   i, i+j-1, n, q), 0, 0);
#else
		prt(format("Message Recall (%d-%d of %d), Offset %d",
		    i, i+j-1, n, q), 0, 0);
#endif


		/* Display prompt (not very informative) */
#ifdef JP
		prt("[ 'p' で更に古いもの, 'n' で更に新しいもの, '/' で検索, ESC で中断 ]", hgt - 1, 0);
#else
		prt("[Press 'p' for older, 'n' for newer, ..., or ESCAPE]", hgt - 1, 0);
#endif


		/* Get a command */
		k = inkey();

		/* Exit on Escape */
		if (k == ESCAPE) break;

		/* Hack -- Save the old index */
		j = i;

		/* Horizontal scroll */
		if (k == '4')
		{
			/* Scroll left */
			q = (q >= 40) ? (q - 40) : 0;

			/* Success */
			continue;
		}

		/* Horizontal scroll */
		if (k == '6')
		{
			/* Scroll right */
			q = q + 40;

			/* Success */
			continue;
		}

		/* Hack -- handle show */
		if (k == '=')
		{
			/* Prompt */
#ifdef JP
			prt("強調: ", hgt - 1, 0);
#else
			prt("Show: ", hgt - 1, 0);
#endif


			/* Get a "shower" string, or continue */
			if (!askfor_aux(shower, 80)) continue;

			/* Okay */
			continue;
		}

		/* Hack -- handle find */
		if (k == '/')
		{
			int z;

			/* Prompt */
#ifdef JP
			prt("検索: ", hgt - 1, 0);
#else
			prt("Find: ", hgt - 1, 0);
#endif


			/* Get a "finder" string, or continue */
			if (!askfor_aux(finder, 80)) continue;

			/* Show it */
			strcpy(shower, finder);

			/* Scan messages */
			for (z = i + 1; z < n; z++)
			{
				cptr msg = message_str(z);

				/* Search for it */
				if (strstr(msg, finder))
				{
					/* New location */
					i = z;

					/* Done */
					break;
				}
			}
		}

		/* Recall 1 older message */
		if ((k == '8') || (k == '\n') || (k == '\r'))
		{
			/* Go newer if legal */
			if (i + 1 < n) i += 1;
		}

		/* Recall 10 older messages */
		if (k == '+')
		{
			/* Go older if legal */
			if (i + 10 < n) i += 10;
		}

		/* Recall 20 older messages */
		if ((k == 'p') || (k == KTRL('P')) || (k == ' '))
		{
			/* Go older if legal */
			if (i + 20 < n) i += 20;
		}

		/* Recall 20 newer messages */
		if ((k == 'n') || (k == KTRL('N')))
		{
			/* Go newer (if able) */
			i = (i >= 20) ? (i - 20) : 0;
		}

		/* Recall 10 newer messages */
		if (k == '-')
		{
			/* Go newer (if able) */
			i = (i >= 20) ? (i - 20) : 0;
		}

		/* Recall 1 newer messages */
		if (k == '2')
		{
			/* Go newer (if able) */
			i = (i >= 1) ? (i - 1) : 0;
		}

		/* Hack -- Error of some kind */
		if (i == j) bell();
	}

	/* Restore the screen */
	screen_load();
}



/*
 * Number of cheating options
 */
#define CHEAT_MAX 7

/*
 * Cheating options
 */
static option_type cheat_info[CHEAT_MAX] =
{
	{ &cheat_peek,		FALSE,	255,	0x01, 0x00,
#ifdef JP
	"cheat_peek",		"アイテムの生成をのぞき見る"
#else
	"cheat_peek",		"Peek into object creation"
#endif
	},

	{ &cheat_hear,		FALSE,	255,	0x02, 0x00,
#ifdef JP
	"cheat_hear",		"モンスターの生成をのぞき見る"
#else
	"cheat_hear",		"Peek into monster creation"
#endif
	},

	{ &cheat_room,		FALSE,	255,	0x04, 0x00,
#ifdef JP
	"cheat_room",		"ダンジョンの生成をのぞき見る"
#else
	"cheat_room",		"Peek into dungeon creation"
#endif
	},

	{ &cheat_xtra,		FALSE,	255,	0x08, 0x00,
#ifdef JP
	"cheat_xtra",		"その他の事をのぞき見る"
#else
	"cheat_xtra",		"Peek into something else"
#endif
	},

	{ &cheat_know,		FALSE,	255,	0x10, 0x00,
#ifdef JP
	"cheat_know",		"完全なモンスターの思い出を知る"
#else
	"cheat_know",		"Know complete monster info"
#endif
	},

	{ &cheat_live,		FALSE,	255,	0x20, 0x00,
#ifdef JP
	"cheat_live",		"死を回避することを可能にする"
#else
	"cheat_live",		"Allow player to avoid death"
#endif
	},

	{ &cheat_save,		FALSE,	255,	0x40, 0x00,
#ifdef JP
	"cheat_save",		"死んだ時セーブするか確認する"
#else
	"cheat_save",		"Ask for saving death"
#endif
	}
};

/*
 * Interact with some options for cheating
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
#ifdef JP
		sprintf(buf, "%s ( リターンで次へ, y/n でセット, ESC で決定 )", info);
#else
		sprintf(buf, "%s (RET to advance, y/n to set, ESC to accept) ", info);
#endif

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
#ifdef JP
			    (*cheat_info[i].o_var ? "はい  " : "いいえ"),
#else
			    (*cheat_info[i].o_var ? "yes" : "no "),
#endif

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
#ifdef JP
					do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "詐欺オプションをONにして、スコアを残せなくなった。");
#else
					do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "give up sending score to use cheating options.");
#endif
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
#ifdef JP
				strnfmt(buf, sizeof(buf), "joption.txt#%s", cheat_info[k].o_text);
#else
				strnfmt(buf, sizeof(buf), "option.txt#%s", cheat_info[k].o_text);
#endif
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


static option_type autosave_info[2] =
{
	{ &autosave_l,      FALSE, 255, 0x01, 0x00,
#ifdef JP
	    "autosave_l",    "新しい階に入る度に自動セーブする" },
#else
	    "autosave_l",    "Autosave when entering new levels" },
#endif


	{ &autosave_t,      FALSE, 255, 0x02, 0x00,
#ifdef JP
	    "autosave_t",   "一定ターン毎に自動セーブする" },
#else
	    "autosave_t",   "Timed autosave" },
#endif

};


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


/*
 * Interact with some options for cheating
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
#ifdef JP
		sprintf(buf, "%s ( リターンで次へ, y/n でセット, F で頻度を入力, ESC で決定 ) ", info);
#else
		sprintf(buf, "%s (RET to advance, y/n to set, 'F' for frequency, ESC to accept) ", info);
#endif

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
#ifdef JP
			    (*autosave_info[i].o_var ? "はい  " : "いいえ"),
#else
			    (*autosave_info[i].o_var ? "yes" : "no "),
#endif

			    autosave_info[i].o_text);
			c_prt(a, buf, i + 2, 0);
		}

#ifdef JP
		prt(format("自動セーブの頻度： %d ターン毎",  autosave_freq), 5, 0);
#else
		prt(format("Timed autosave frequency: every %d turns",  autosave_freq), 5, 0);
#endif



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
#ifdef JP
				prt(format("自動セーブの頻度： %d ターン毎", 
					   autosave_freq), 5, 0);
#else
				prt(format("Timed autosave frequency: every %d turns",
					   autosave_freq), 5, 0);
#endif
				break;
			}

			case '?':
			{
#ifdef JP
				(void)show_file(TRUE, "joption.txt#Autosave", NULL, 0, 0);
#else
				(void)show_file(TRUE, "option.txt#Autosave", NULL, 0, 0);
#endif


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


#define PAGE_AUTODESTROY 7

/*
 * Interact with some options
 */
void do_cmd_options_aux(int page, cptr info)
{
	char    ch;
	int     i, k = 0, n = 0, l;
	int     opt[24];
	char    buf[80];


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
#ifdef JP
		sprintf(buf, "%s (リターン:次, y/n:変更, ESC:終了, ?:ヘルプ) ", info);
#else
		sprintf(buf, "%s (RET:next, y/n:change, ESC:accept, ?:help) ", info);
#endif

		prt(buf, 0, 0);


		/* HACK -- description for easy-auto-destroy options */
#ifdef JP
		if (page == PAGE_AUTODESTROY) c_prt(TERM_YELLOW, "以下のオプションは、簡易自動破壊を使用するときのみ有効", 6, 6);
#else
		if (page == PAGE_AUTODESTROY) c_prt(TERM_YELLOW, "Following options will protect items from easy auto-destroyer.", 6, 3);
#endif

		/* Display the options */
		for (i = 0; i < n; i++)
		{
			byte a = TERM_WHITE;

			/* Color current option */
			if (i == k) a = TERM_L_BLUE;

			/* Display the option text */
			sprintf(buf, "%-48s: %s (%.19s)",
				option_info[opt[i]].o_desc,
#ifdef JP
				(*option_info[opt[i]].o_var ? "はい  " : "いいえ"),
#else
				(*option_info[opt[i]].o_var ? "yes" : "no "),
#endif

				option_info[opt[i]].o_text);
			if ((page == PAGE_AUTODESTROY) && i > 2) c_prt(a, buf, i + 5, 0);
			else c_prt(a, buf, i + 2, 0);
		}

		if ((page == PAGE_AUTODESTROY) && (k > 2)) l = 3;
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
				(*option_info[opt[k]].o_var) = TRUE;
				k = (k + 1) % n;
				break;
			}

			case 'n':
			case 'N':
			case '4':
			{
				(*option_info[opt[k]].o_var) = FALSE;
				k = (k + 1) % n;
				break;
			}

			case 't':
			case 'T':
			{
				(*option_info[opt[k]].o_var) = !(*option_info[opt[k]].o_var);
				break;
			}

			case '?':
			{
#ifdef JP
				strnfmt(buf, sizeof(buf), "joption.txt#%s", option_info[opt[k]].o_text);
#else
				strnfmt(buf, sizeof(buf), "option.txt#%s", option_info[opt[k]].o_text);
#endif
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


/*
 * Modify the "window" options
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
#ifdef JP
		prt("ウィンドウ・フラグ (<方向>で移動, tでチェンジ, y/n でセット, ESC)", 0, 0);
#else
		prt("Window Flags (<dir>, t, y, n, ESC) ", 0, 0);
#endif


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
#ifdef JP
			if (!str) str = "(未使用)";
#else
			if (!str) str = "(Unused option)";
#endif


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
#ifdef JP
				(void)show_file(TRUE, "joption.txt#Window", NULL, 0, 0);
#else
				(void)show_file(TRUE, "option.txt#Window", NULL, 0, 0);
#endif


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




/*
 * Set or unset various options.
 *
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 */
void do_cmd_options(void)
{
	int k;


	/* Save the screen */
	screen_save();

	/* Interact */
	while (1)
	{
		/* Clear screen */
		Term_clear();

		/* Why are we here */
#ifdef JP
		prt("[ オプションの設定 ]", 2, 0);
#else
		prt("Options", 2, 0);
#endif


		/* Give some choices */
#ifdef JP
		prt("(1)     キー入力          オプション", 4, 5);
		prt("(2)     画面出力          オプション", 5, 5);
		prt("(3)   ゲームプレイ        オプション", 6, 5);
		prt("(4)   行動中止関係        オプション", 7, 5);
		prt("(5)      効率化           オプション", 8, 5);
		prt("(6) 簡易アイテム自動破壊  オプション", 9, 5);
		prt("(R)    プレイ記録         オプション", 10, 5);
		/* Special choices */
		prt("(D)  基本ウェイト量", 12, 5);
		prt("(H) 低ヒットポイント警告", 13, 5);
		prt("(A)    自動セーブ         オプション", 14, 5);
		/* Window flags */
		prt("(W) ウインドウフラグ", 15, 5);
#else
		prt("(1) Input Options", 4, 5);
		prt("(2) Output Options", 5, 5);
		prt("(3) Game-Play Options", 6, 5);
		prt("(4) Disturbance Options", 7, 5);
		prt("(5) Efficiency Options", 8, 5);
		prt("(6) Easy Auto-Destroyer Options", 9, 5);
		prt("(R) Play-record Options", 10, 5);

		/* Special choices */
		prt("(D) Base Delay Factor", 12, 5);
		prt("(H) Hitpoint Warning", 13, 5);
		prt("(A) Autosave Options", 14, 5);


		/* Window flags */
		prt("(W) Window Flags", 15, 5);
#endif

		if (p_ptr->noscore || allow_debug_opts)
		{
			/* Cheating */
#ifdef JP
			prt("(C)       詐欺            オプション", 16, 5);
#else
			prt("(C) Cheating Options", 16, 5);
#endif
		}


		/* Prompt */
#ifdef JP
		prt("コマンド:", 18, 0);
#else
		prt("Command: ", 18, 0);
#endif


		/* Get command */
		k = inkey();

		/* Exit */
		if (k == ESCAPE) break;

		/* Analyze */
		switch (k)
		{
			/* General Options */
			case '1':
			{
				/* Process the general options */
#ifdef JP
				do_cmd_options_aux(1, "キー入力オプション");
#else
				do_cmd_options_aux(1, "Input Options");
#endif

				break;
			}

			/* General Options */
			case '2':
			{
				/* Process the general options */
#ifdef JP
				do_cmd_options_aux(2, "画面出力オプション");
#else
				do_cmd_options_aux(2, "Output Options");
#endif

				break;
			}

			/* Inventory Options */
			case '3':
			{
				/* Spawn */
#ifdef JP
				do_cmd_options_aux(3, "ゲームプレイ・オプション");
#else
				do_cmd_options_aux(3, "Game-Play Options");
#endif

				break;
			}

			/* Disturbance Options */
			case '4':
			{
				/* Spawn */
#ifdef JP
				do_cmd_options_aux(4, "行動中止関係のオプション");
#else
				do_cmd_options_aux(4, "Disturbance Options");
#endif

				break;
			}

			/* Efficiency Options */
			case '5':
			{
				/* Spawn */
#ifdef JP
				do_cmd_options_aux(5, "効率化オプション");
#else
				do_cmd_options_aux(5, "Efficiency Options");
#endif

				break;
			}

			/* Object auto-destruction Options */
			case '6':
			{
				/* Spawn */
#ifdef JP
				do_cmd_options_aux(7, "簡易アイテム自動破壊オプション");
#else
				do_cmd_options_aux(7, "Easy Auto-Destroyer Options");
#endif
				break;
			}

			/* Play-record Options */
			case 'R':
			case 'r':
			{
				/* Spawn */
#ifdef JP
				do_cmd_options_aux(10, "プレイ記録オプション");
#else
				do_cmd_options_aux(10, "Play-record Option");
#endif
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
#ifdef JP
				do_cmd_options_cheat("詐欺師は決して勝利できない！");
#else
				do_cmd_options_cheat("Cheaters never win");
#endif

				break;
			}

			case 'a':
			case 'A':
			{
#ifdef JP
				do_cmd_options_autosave("自動セーブ");
#else
				do_cmd_options_autosave("Autosave");
#endif

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
						  PW_BORG_1 | PW_BORG_2 | PW_DUNGEON);
				break;
			}

			/* Hack -- Delay Speed */
			case 'D':
			case 'd':
			{
				/* Prompt */
#ifdef JP
				prt("コマンド: 基本ウェイト量", 18, 0);
#else
				prt("Command: Base Delay Factor", 18, 0);
#endif


				/* Get a new value */
				while (1)
				{
					int msec = delay_factor * delay_factor * delay_factor;
#ifdef JP
					prt(format("現在のウェイト: %d (%dミリ秒)",
						   delay_factor, msec), 22, 0);
#else
					prt(format("Current base delay factor: %d (%d msec)",
						   delay_factor, msec), 22, 0);
#endif

#ifdef JP
					prt("ウェイト (0-9) ESCで決定: ", 20, 0);
#else
					prt("Delay Factor (0-9 or ESC to accept): ", 20, 0);
#endif

					k = inkey();
					if (k == ESCAPE) break;
					else if (k == '?')
					{
#ifdef JP
						(void)show_file(TRUE, "joption.txt#BaseDelay", NULL, 0, 0);
#else
						(void)show_file(TRUE, "option.txt#BaseDelay", NULL, 0, 0);
#endif
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
#ifdef JP
				prt("コマンド: 低ヒットポイント警告", 18, 0);
#else
				prt("Command: Hitpoint Warning", 18, 0);
#endif


				/* Get a new value */
				while (1)
				{
#ifdef JP
					prt(format("現在の低ヒットポイント警告: %d0%%",
						   hitpoint_warn), 22, 0);
#else
					prt(format("Current hitpoint warning: %d0%%",
						   hitpoint_warn), 22, 0);
#endif

#ifdef JP
					prt("低ヒットポイント警告 (0-9) ESCで決定: ", 20, 0);
#else
					prt("Hitpoint Warning (0-9 or ESC to accept): ", 20, 0);
#endif

					k = inkey();
					if (k == ESCAPE) break;
					else if (k == '?')
					{
#ifdef JP
						(void)show_file(TRUE, "joption.txt#Hitpoint", NULL, 0, 0);
#else
						(void)show_file(TRUE, "option.txt#Hitpoint", NULL, 0, 0);
#endif
						Term_clear(); 
					}
					else if (isdigit(k)) hitpoint_warn = D2I(k);
					else bell();
				}

				break;
			}

			case '?':
#ifdef JP
				(void)show_file(TRUE, "joption.txt", NULL, 0, 0);
#else
				(void)show_file(TRUE, "option.txt", NULL, 0, 0);
#endif
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



/*
 * Ask for a "user pref line" and process it
 *
 * XXX XXX XXX Allow absolute file names?
 */
void do_cmd_pref(void)
{
	char buf[80];

	/* Default */
	strcpy(buf, "");

	/* Ask for a "user pref command" */
#ifdef JP
	if (!get_string("設定変更コマンド: ", buf, 80)) return;
#else
	if (!get_string("Pref: ", buf, 80)) return;
#endif


	/* Process that pref command */
	(void)process_pref_file_command(buf);
}

void do_cmd_pickpref(void)
{
	char buf[80];
	errr err;

#ifdef JP
	if(!get_check("自動拾い設定ファイルをロードしますか? ")) return;
#else
	if(!get_check("Reload auto-pick preference file? ")) return;
#endif

	/* Free old entries */
	init_autopicker();

	/* キャラ毎の設定ファイルの読み込み */
#ifdef JP
	sprintf(buf, "picktype-%s.prf", player_name);
#else
	sprintf(buf, "pickpref-%s.prf", player_name);
#endif
	err = process_pickpref_file(buf);

	if(err == 0)
	{
#ifdef JP
		msg_format("%sを読み込みました。", buf);
#else
		msg_format("loaded '%s'.", buf);
#endif
	}

	/* 共通の設定ファイル読み込み */

	/* Process 'pick????.prf' if 'pick????-<name>.prf' doesn't exist */
	if (0 > err)
	{
#ifdef JP
		err = process_pickpref_file("picktype.prf");
#else
		err = process_pickpref_file("pickpref.prf");
#endif

		if(err == 0)
		{
#ifdef JP
			msg_print("picktype.prfを読み込みました。");
#else
			msg_print("loaded 'pickpref.prf'.");
#endif
		}
	}


#ifdef JP
	if(err) msg_print("自動拾い設定ファイルの読み込みに失敗しました。");
#else
	if(err) msg_print("Failed to reload autopick preference.");
#endif
}

#ifdef ALLOW_MACROS

/*
 * Hack -- append all current macros to the given file
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
#ifdef JP
	auto_dump_printf("\n# 自動マクロセーブ\n\n");
#else
	auto_dump_printf("\n# Automatic macro dump\n\n");
#endif

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


/*
 * Hack -- ask for a "trigger" (see below)
 *
 * Note the complex use of the "inkey()" function from "util.c".
 *
 * Note that both "flush()" calls are extremely important.
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


/*
 * Hack -- ask for a keymap "trigger" (see below)
 *
 * Note that both "flush()" calls are extremely important.  This may
 * no longer be true, since "util.c" is much simpler now.  XXX XXX XXX
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


/*
 * Hack -- append all keymaps to the given file
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
#ifdef JP
	auto_dump_printf("\n# 自動キー配置セーブ\n\n");
#else
	auto_dump_printf("\n# Automatic keymap dump\n\n");
#endif

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



/*
 * Interact with "macros"
 *
 * Note that the macro "action" must be defined before the trigger.
 *
 * Could use some helpful instructions on this page.  XXX XXX XXX
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
#ifdef JP
		prt("[ マクロの設定 ]", 2, 0);
#else
		prt("Interact with Macros", 2, 0);
#endif



		/* Describe that action */
#ifdef JP
		prt("(1) ユーザー設定ファイルのロード", 4, 5);
#else
		prt("Current action (if any) shown below:", 20, 0);
#endif


		/* Analyze the current action */
		ascii_to_text(buf, macro__buf);

		/* Display the current action */
		prt(buf, 22, 0);


		/* Selections */
#ifdef JP
		prt("(1) ユーザー設定ファイルのロード", 4, 5);
#else
		prt("(1) Load a user pref file", 4, 5);
#endif

#ifdef ALLOW_MACROS
#ifdef JP
		prt("(2) ファイルにマクロを追加", 5, 5);
		prt("(3) マクロの確認", 6, 5);
		prt("(4) マクロの作成", 7, 5);
		prt("(5) マクロの削除", 8, 5);
		prt("(6) ファイルにキー配置を追加", 9, 5);
		prt("(7) キー配置の確認", 10, 5);
		prt("(8) キー配置の作成", 11, 5);
		prt("(9) キー配置の削除", 12, 5);
		prt("(0) マクロ行動の入力", 13, 5);
#else
		prt("(2) Append macros to a file", 5, 5);
		prt("(3) Query a macro", 6, 5);
		prt("(4) Create a macro", 7, 5);
		prt("(5) Remove a macro", 8, 5);
		prt("(6) Append keymaps to a file", 9, 5);
		prt("(7) Query a keymap", 10, 5);
		prt("(8) Create a keymap", 11, 5);
		prt("(9) Remove a keymap", 12, 5);
		prt("(0) Enter a new action", 13, 5);
#endif

#endif /* ALLOW_MACROS */

		/* Prompt */
#ifdef JP
		prt("コマンド: ", 16, 0);
#else
		prt("Command: ", 16, 0);
#endif


		/* Get a command */
		i = inkey();

		/* Leave */
		if (i == ESCAPE) break;

		/* Load a 'macro' file */
		else if (i == '1')
		{
			errr err;

			/* Prompt */
#ifdef JP
			prt("コマンド: ユーザー設定ファイルのロード", 16, 0);
#else
			prt("Command: Load a user pref file", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 18, 0);
#else
			prt("File: ", 18, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Ask for a file */
			if (!askfor_aux(tmp, 80)) continue;

			/* Process the given filename */
			err = process_pref_file(tmp);
			if (-2 == err)
			{
#ifdef JP
				msg_format("標準の設定ファイル'%s'を読み込みました。", tmp);
#else
				msg_format("Loaded default '%s'.", tmp);
#endif
			}
			else if (err)
			{
				/* Prompt */
#ifdef JP
				msg_format("'%s'の読み込みに失敗しました！", tmp);
#else
				msg_format("Failed to load '%s'!");
#endif
			}
			else
			{
#ifdef JP
				msg_format("'%s'を読み込みました。", tmp);
#else
				msg_format("Loaded '%s'.", tmp);
#endif
			}
		}

#ifdef ALLOW_MACROS

		/* Save macros */
		else if (i == '2')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: マクロをファイルに追加する", 16, 0);
#else
			prt("Command: Append macros to a file", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 18, 0);
#else
			prt("File: ", 18, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Ask for a file */
			if (!askfor_aux(tmp, 80)) continue;

			/* Dump the macros */
			(void)macro_dump(tmp);

			/* Prompt */
#ifdef JP
			msg_print("マクロを追加しました。");
#else
			msg_print("Appended macros.");
#endif

		}

		/* Query a macro */
		else if (i == '3')
		{
			int k;

			/* Prompt */
#ifdef JP
			prt("コマンド: マクロの確認", 16, 0);
#else
			prt("Command: Query a macro", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("トリガーキー: ", 18, 0);
#else
			prt("Trigger: ", 18, 0);
#endif


			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Acquire action */
			k = macro_find_exact(buf);

			/* Nothing found */
			if (k < 0)
			{
				/* Prompt */
#ifdef JP
				msg_print("そのキーにはマクロは定義されていません。");
#else
				msg_print("Found no macro.");
#endif

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
#ifdef JP
				msg_print("マクロを確認しました。");
#else
				msg_print("Found a macro.");
#endif

			}
		}

		/* Create a macro */
		else if (i == '4')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: マクロの作成", 16, 0);
#else
			prt("Command: Create a macro", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("トリガーキー: ", 18, 0);
#else
			prt("Trigger: ", 18, 0);
#endif


			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Clear */
			clear_from(20);

			/* Prompt */
#ifdef JP
			prt("マクロ行動: ", 20, 0);
#else
			prt("Action: ", 20, 0);
#endif


			/* Convert to text */
			ascii_to_text(tmp, macro__buf);

			/* Get an encoded action */
			if (askfor_aux(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro__buf, tmp);

				/* Link the macro */
				macro_add(buf, macro__buf);

				/* Prompt */
#ifdef JP
				msg_print("マクロを追加しました。");
#else
				msg_print("Added a macro.");
#endif

			}
		}

		/* Remove a macro */
		else if (i == '5')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: マクロの削除", 16, 0);
#else
			prt("Command: Remove a macro", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("トリガーキー: ", 18, 0);
#else
			prt("Trigger: ", 18, 0);
#endif


			/* Get a macro trigger */
			do_cmd_macro_aux(buf);

			/* Link the macro */
			macro_add(buf, buf);

			/* Prompt */
#ifdef JP
			msg_print("マクロを削除しました。");
#else
			msg_print("Removed a macro.");
#endif

		}

		/* Save keymaps */
		else if (i == '6')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: キー配置をファイルに追加する", 16, 0);
#else
			prt("Command: Append keymaps to a file", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 18, 0);
#else
			prt("File: ", 18, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Ask for a file */
			if (!askfor_aux(tmp, 80)) continue;

			/* Dump the macros */
			(void)keymap_dump(tmp);

			/* Prompt */
#ifdef JP
			msg_print("キー配置を追加しました。");
#else
			msg_print("Appended keymaps.");
#endif

		}

		/* Query a keymap */
		else if (i == '7')
		{
			cptr act;

			/* Prompt */
#ifdef JP
			prt("コマンド: キー配置の確認", 16, 0);
#else
			prt("Command: Query a keymap", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("押すキー: ", 18, 0);
#else
			prt("Keypress: ", 18, 0);
#endif


			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Look up the keymap */
			act = keymap_act[mode][(byte)(buf[0])];

			/* Nothing found */
			if (!act)
			{
				/* Prompt */
#ifdef JP
				msg_print("キー配置は定義されていません。");
#else
				msg_print("Found no keymap.");
#endif

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
#ifdef JP
				msg_print("キー配置を確認しました。");
#else
				msg_print("Found a keymap.");
#endif

			}
		}

		/* Create a keymap */
		else if (i == '8')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: キー配置の作成", 16, 0);
#else
			prt("Command: Create a keymap", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("押すキー: ", 18, 0);
#else
			prt("Keypress: ", 18, 0);
#endif


			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Clear */
			clear_from(20);

			/* Prompt */
#ifdef JP
			prt("行動: ", 20, 0);
#else
			prt("Action: ", 20, 0);
#endif


			/* Convert to text */
			ascii_to_text(tmp, macro__buf);

			/* Get an encoded action */
			if (askfor_aux(tmp, 80))
			{
				/* Convert to ascii */
				text_to_ascii(macro__buf, tmp);

				/* Free old keymap */
				string_free(keymap_act[mode][(byte)(buf[0])]);

				/* Make new keymap */
				keymap_act[mode][(byte)(buf[0])] = string_make(macro__buf);

				/* Prompt */
#ifdef JP
				msg_print("キー配置を追加しました。");
#else
				msg_print("Added a keymap.");
#endif

			}
		}

		/* Remove a keymap */
		else if (i == '9')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: キー配置の削除", 16, 0);
#else
			prt("Command: Remove a keymap", 16, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("押すキー: ", 18, 0);
#else
			prt("Keypress: ", 18, 0);
#endif


			/* Get a keymap trigger */
			do_cmd_macro_aux_keymap(buf);

			/* Free old keymap */
			string_free(keymap_act[mode][(byte)(buf[0])]);

			/* Make new keymap */
			keymap_act[mode][(byte)(buf[0])] = NULL;

			/* Prompt */
#ifdef JP
			msg_print("キー配置を削除しました。");
#else
			msg_print("Removed a keymap.");
#endif

		}

		/* Enter a new action */
		else if (i == '0')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: マクロ行動の入力", 16, 0);
#else
			prt("Command: Enter a new action", 16, 0);
#endif


			/* Go to the correct location */
			Term_gotoxy(0, 22);

			/* Hack -- limit the value */
			tmp[80] = '\0';

			/* Get an encoded action */
			if (!askfor_aux(buf, 80)) continue;

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


static void cmd_visuals_aux(int i, int *num, int max)
{
	if (iscntrl(i))
	{
		char str[10] = "";
		int tmp;

		sprintf(str, "%d", *num);

		if (!get_string(format("Input new number(0-%d): ", max-1), str, 4))
			return;

		tmp = strtol(str, NULL, 0);
		if (tmp >= 0 && tmp < max)
			*num = tmp;
		return;
	}
	else if (isupper(i))
		*num = (*num + max - 1) % max;
	else
		*num = (*num + 1) % max;

	return;
}

/*
 * Interact with "visuals"
 */
void do_cmd_visuals(void)
{
	int i;

	char tmp[160];

	char buf[1024];

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
#ifdef JP
		prt("画面表示の設定", 2, 0);
#else
		prt("Interact with Visuals", 2, 0);
#endif


		/* Give some choices */
#ifdef JP
		prt("(1) ユーザー設定ファイルのロード", 4, 5);
#else
		prt("(1) Load a user pref file", 4, 5);
#endif

#ifdef ALLOW_VISUALS
#ifdef JP
		prt("(2) モンスターの 色/文字 をファイルに書き出す", 5, 5);
		prt("(3) アイテムの   色/文字 をファイルに書き出す", 6, 5);
		prt("(4) 地形の       色/文字 をファイルに書き出す", 7, 5);
		prt("(5) (未使用)", 8, 5);
		prt("(6) モンスターの 色/文字 を変更する", 9, 5);
		prt("(7) アイテムの   色/文字 を変更する", 10, 5);
		prt("(8) 地形の       色/文字 を変更する", 11, 5);
		prt("(9) (未使用)", 12, 5);
#else
		prt("(2) Dump monster attr/chars", 5, 5);
		prt("(3) Dump object attr/chars", 6, 5);
		prt("(4) Dump feature attr/chars", 7, 5);
		prt("(5) (unused)", 8, 5);
		prt("(6) Change monster attr/chars", 9, 5);
		prt("(7) Change object attr/chars", 10, 5);
		prt("(8) Change feature attr/chars", 11, 5);
		prt("(9) (unused)", 12, 5);
#endif

#endif
#ifdef JP
		prt("(0) 画面表示方法の初期化", 13, 5);
#else
		prt("(0) Reset visuals", 13, 5);
#endif


		/* Prompt */
#ifdef JP
		prt("コマンド:", 18, 0);
#else
		prt("Command: ", 15, 0);
#endif


		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;

		/* Load a 'pref' file */
		else if (i == '1')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: ユーザー設定ファイルのロード", 15, 0);
#else
			prt("Command: Load a user pref file", 15, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 18, 0);
#else
			prt("File: ", 17, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Query */
			if (!askfor_aux(tmp, 70)) continue;

			/* Process the given filename */
			(void)process_pref_file(tmp);
		}

#ifdef ALLOW_VISUALS

		/* Dump monster attr/chars */
		else if (i == '2')
		{
			static cptr mark = "Monster attr/chars";

			/* Prompt */
#ifdef JP
			prt("コマンド: モンスターの[色/文字]をファイルに書き出します", 15, 0);
#else
			prt("Command: Dump monster attr/chars", 15, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 17, 0);
#else
			prt("File: ", 17, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);
			
			/* Get a filename */
			if (!askfor_aux(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
#ifdef JP
			auto_dump_printf("\n# モンスターの[色/文字]の設定\n\n");
#else
			auto_dump_printf("\n# Monster attr/char definitions\n\n");
#endif

			/* Dump monsters */
			for (i = 1; i < max_r_idx; i++)
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
#ifdef JP
			msg_print("モンスターの[色/文字]をファイルに書き出しました。");
#else
			msg_print("Dumped monster attr/chars.");
#endif

		}

		/* Dump object attr/chars */
		else if (i == '3')
		{
			static cptr mark = "Object attr/chars";

			/* Prompt */
#ifdef JP
			prt("コマンド: アイテムの[色/文字]をファイルに書き出します", 15, 0);
#else
			prt("Command: Dump object attr/chars", 15, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 17, 0);
#else
			prt("File: ", 17, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Get a filename */
			if (!askfor_aux(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
#ifdef JP
			auto_dump_printf("\n# アイテムの[色/文字]の設定\n\n");
#else
			auto_dump_printf("\n# Object attr/char definitions\n\n");
#endif

			/* Dump objects */
			for (i = 1; i < max_k_idx; i++)
			{
				char o_name[80];
				object_kind *k_ptr = &k_info[i];

				/* Skip non-entries */
				if (!k_ptr->name) continue;

				/* Skip entries with flavor */
				if (k_ptr->flavor) continue;

				/* Tidy name */
				strip_name(o_name, i);

				/* Dump a comment */
				auto_dump_printf("# %s\n", o_name);

				/* Dump the object attr/char info */
				auto_dump_printf("K:%d:0x%02X/0x%02X\n\n", i,
					(byte)(k_ptr->x_attr), (byte)(k_ptr->x_char));
			}

			/* Close */
			close_auto_dump();

			/* Message */
#ifdef JP
			msg_print("アイテムの[色/文字]をファイルに書き出しました。");
#else
			msg_print("Dumped object attr/chars.");
#endif

		}

		/* Dump feature attr/chars */
		else if (i == '4')
		{
			static cptr mark = "Feature attr/chars";

			/* Prompt */
#ifdef JP
			prt("コマンド: 地形の[色/文字]をファイルに書き出します", 15, 0);
#else
			prt("Command: Dump feature attr/chars", 15, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 17, 0);
#else
			prt("File: ", 17, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Get a filename */
			if (!askfor_aux(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
#ifdef JP
			auto_dump_printf("\n# 地形の[色/文字]の設定\n\n");
#else
			auto_dump_printf("\n# Feature attr/char definitions\n\n");
#endif

			/* Dump features */
			for (i = 1; i < max_f_idx; i++)
			{
				feature_type *f_ptr = &f_info[i];

				/* Skip non-entries */
				if (!f_ptr->name) continue;

				/* Skip mimiccing features */
				if (f_ptr->mimic != i) continue;

				/* Dump a comment */
				auto_dump_printf("# %s\n", (f_name + f_ptr->name));

				/* Dump the feature attr/char info */
				auto_dump_printf("F:%d:0x%02X/0x%02X\n\n", i,
					(byte)(f_ptr->x_attr), (byte)(f_ptr->x_char));
			}

			/* Close */
			close_auto_dump();

			/* Message */
#ifdef JP
			msg_print("地形の[色/文字]をファイルに書き出しました。");
#else
			msg_print("Dumped feature attr/chars.");
#endif

		}

		/* Modify monster attr/chars */
		else if (i == '6')
		{
			static int r = 0;

			/* Prompt */
#ifdef JP
			prt("コマンド: モンスターの[色/文字]を変更します", 15, 0);
#else
			prt("Command: Change monster attr/chars", 15, 0);
#endif


			/* Hack -- query until done */
			while (1)
			{
				monster_race *r_ptr = &r_info[r];
				byte a, a2;
				char c, c2;
				int t;

				byte da = (r_ptr->d_attr);
				byte dc = (r_ptr->d_char);
				byte ca = (r_ptr->x_attr);
				byte cc = (r_ptr->x_char);

				/* Label the object */
#ifdef JP
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("モンスター = %d, 名前 = %-40.40s",
						   r, (r_name + r_ptr->name)));
#else
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Monster = %d, Name = %-40.40s",
						   r, (r_name + r_ptr->name)));
#endif


				/* Label the Default values */
#ifdef JP
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("初期値  色 / 文字 = %3u / %3u", da, dc));
#else
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3u / %3u", da, dc));
#endif

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);

				a = da;
				c = dc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 19, a, c);
				if (use_bigtile) Term_putch(43 + 1, 19, a2, c2);

				/* Label the Current values */
#ifdef JP
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("現在値  色 / 文字 = %3u / %3u", ca, cc));
#else
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("Current attr/char = %3u / %3u", ca, cc));
#endif

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);

				a = ca;
				c = cc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 20, a, c);
				if (use_bigtile) Term_putch(43 + 1, 20, a2, c2);


				/* Prompt */
#ifdef JP
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "コマンド (n/N/^N/a/A/^A/c/C/^C): ");
#else
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "Command (n/N/^N/a/A/^A/c/C/^C): ");
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
					cmd_visuals_aux(i, &r, max_r_idx);
					break;
				case 'a':
					t = (int)r_ptr->x_attr;
					cmd_visuals_aux(i, &t, 256);
					r_ptr->x_attr = (byte)t;
					break;
				case 'c':
					t = (int)r_ptr->x_char;
					cmd_visuals_aux(i, &t, 256);
					r_ptr->x_char = (byte)t;
					break;
				}
			}
		}

		/* Modify object attr/chars */
		else if (i == '7')
		{
			static int k = 0;

			/* Prompt */
#ifdef JP
			prt("コマンド: アイテムの[色/文字]を変更します", 15, 0);
#else
			prt("Command: Change object attr/chars", 15, 0);
#endif


			/* Hack -- query until done */
			while (1)
			{
				object_kind *k_ptr = &k_info[k];
				byte a, a2;
				char c, c2;
				int t;

				byte da = (byte)k_ptr->d_attr;
				byte dc = (byte)k_ptr->d_char;
				byte ca = (byte)k_ptr->x_attr;
				byte cc = (byte)k_ptr->x_char;

				/* Label the object */
#ifdef JP
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("アイテム = %d, 名前 = %-40.40s",
						   k, (k_name + k_ptr->name)));
#else
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Object = %d, Name = %-40.40s",
						   k, (k_name + k_ptr->name)));
#endif


				/* Label the Default values */
#ifdef JP
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("初期値  色 / 文字 = %3d / %3d", da, dc));
#else
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3d / %3d", da, dc));
#endif

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				a = da;
				c = dc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 19, a, c);
				if (use_bigtile) Term_putch(43 + 1, 19, a2, c2);


				/* Label the Current values */
#ifdef JP
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("現在値  色 / 文字 = %3d / %3d", ca, cc));
#else
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("Current attr/char = %3d / %3d", ca, cc));
#endif

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
				a = ca;
				c = cc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 20, a, c);
				if (use_bigtile) Term_putch(43 + 1, 20, a2, c2);


				/* Prompt */
#ifdef JP
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "コマンド (n/N/^N/a/A/^A/c/C/^C): ");
#else
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "Command (n/N/^N/a/A/^A/c/C/^C): ");
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
					cmd_visuals_aux(i, &k, max_k_idx);
					break;
				case 'a':
					t = (int)k_info[k].x_attr;
					cmd_visuals_aux(i, &t, 256);
					k_info[k].x_attr = (byte)t;
					break;
				case 'c':
					t = (int)k_info[k].x_char;
					cmd_visuals_aux(i, &t, 256);
					k_info[k].x_char = (byte)t;
					break;
				}
			}
		}

		/* Modify feature attr/chars */
		else if (i == '8')
		{
			static int f = 0;

			/* Prompt */
#ifdef JP
			prt("コマンド: 地形の[色/文字]を変更します", 15, 0);
#else
			prt("Command: Change feature attr/chars", 15, 0);
#endif


			/* Hack -- query until done */
			while (1)
			{
				feature_type *f_ptr = &f_info[f];
				byte a, a2;
				char c, c2;
				int t;

				byte da = (byte)f_ptr->d_attr;
				byte dc = (byte)f_ptr->d_char;
				byte ca = (byte)f_ptr->x_attr;
				byte cc = (byte)f_ptr->x_char;

				/* Label the object */
#ifdef JP
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("地形 = %d, 名前 = %-40.40s",
						   f, (f_name + f_ptr->name)));
#else
				Term_putstr(5, 17, -1, TERM_WHITE,
					    format("Terrain = %d, Name = %-40.40s",
						   f, (f_name + f_ptr->name)));
#endif


				/* Label the Default values */
#ifdef JP
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("初期値  色 / 文字 = %3d / %3d", da, dc));
#else
				Term_putstr(10, 19, -1, TERM_WHITE,
					    format("Default attr/char = %3d / %3d", da, dc));
#endif

				Term_putstr(40, 19, -1, TERM_WHITE, empty_symbol);
				a = da;
				c = dc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 19, a, c);
				if (use_bigtile) Term_putch(43 + 1, 19, a2, c2);


				/* Label the Current values */
#ifdef JP
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("現在値  色 / 文字 = %3d / %3d", ca, cc));
#else
				Term_putstr(10, 20, -1, TERM_WHITE,
					    format("Current attr/char = %3d / %3d", ca, cc));
#endif

				Term_putstr(40, 20, -1, TERM_WHITE, empty_symbol);
				a = ca;
				c = cc;
				if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

				Term_putch(43, 20, a, c);
				if (use_bigtile) Term_putch(43 + 1, 20, a2, c2);


				/* Prompt */
#ifdef JP
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "コマンド (n/N/^N/a/A/^A/c/C/^C): ");
#else
				Term_putstr(0, 22, -1, TERM_WHITE,
					    "Command (n/N/^N/a/A/^A/c/C/^C): ");
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
					cmd_visuals_aux(i, &f, max_f_idx);
					break;
				case 'a':
					t = (int)f_info[f].x_attr;
					cmd_visuals_aux(i, &t, 256);
					f_info[f].x_attr = (byte)t;
					break;
				case 'c':
					t = (int)f_info[f].x_char;
					cmd_visuals_aux(i, &t, 256);
					f_info[f].x_char = (byte)t;
					break;
				}
			}
		}

#endif

		/* Reset visuals */
		else if (i == '0')
		{
			/* Reset */
			reset_visuals();

			/* Message */
#ifdef JP
			msg_print("画面上の[色/文字]を初期値にリセットしました。");
#else
			msg_print("Visual attr/char tables reset.");
#endif

		}

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
#ifdef JP
		prt("[ カラーの設定 ]", 2, 0);
#else
		prt("Interact with Colors", 2, 0);
#endif


		/* Give some choices */
#ifdef JP
		prt("(1) ユーザー設定ファイルのロード", 4, 5);
#else
		prt("(1) Load a user pref file", 4, 5);
#endif

#ifdef ALLOW_COLORS
#ifdef JP
		prt("(2) カラーの設定をファイルに書き出す", 5, 5);
		prt("(3) カラーの設定を変更する", 6, 5);
#else
		prt("(2) Dump colors", 5, 5);
		prt("(3) Modify colors", 6, 5);
#endif

#endif

		/* Prompt */
#ifdef JP
		prt("コマンド: ", 8, 0);
#else
		prt("Command: ", 8, 0);
#endif


		/* Prompt */
		i = inkey();

		/* Done */
		if (i == ESCAPE) break;

		/* Load a 'pref' file */
		if (i == '1')
		{
			/* Prompt */
#ifdef JP
			prt("コマンド: ユーザー設定ファイルをロードします", 8, 0);
#else
			prt("Command: Load a user pref file", 8, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 10, 0);
#else
			prt("File: ", 10, 0);
#endif


			/* Default file */
			sprintf(tmp, "%s.prf", player_name);

			/* Query */
			if (!askfor_aux(tmp, 70)) continue;

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
#ifdef JP
			prt("コマンド: カラーの設定をファイルに書き出します", 8, 0);
#else
			prt("Command: Dump colors", 8, 0);
#endif


			/* Prompt */
#ifdef JP
			prt("ファイル: ", 10, 0);
#else
			prt("File: ", 10, 0);
#endif


			/* Default filename */
			sprintf(tmp, "%s.prf", player_name);

			/* Get a filename */
			if (!askfor_aux(tmp, 70)) continue;

			/* Build the filename */
			path_build(buf, sizeof(buf), ANGBAND_DIR_USER, tmp);

			/* Append to the file */
			if (!open_auto_dump(buf, mark)) continue;

			/* Start dumping */
#ifdef JP
			auto_dump_printf("\n# カラーの設定\n\n");
#else
			auto_dump_printf("\n# Color redefinitions\n\n");
#endif

			/* Dump colors */
			for (i = 0; i < 256; i++)
			{
				int kv = angband_color_table[i][0];
				int rv = angband_color_table[i][1];
				int gv = angband_color_table[i][2];
				int bv = angband_color_table[i][3];

#ifdef JP
				cptr name = "未知";
#else
				cptr name = "unknown";
#endif


				/* Skip non-entries */
				if (!kv && !rv && !gv && !bv) continue;

				/* Extract the color name */
				if (i < 16) name = color_names[i];

				/* Dump a comment */
#ifdef JP
				auto_dump_printf("# カラー '%s'\n", name);
#else
				auto_dump_printf("# Color '%s'\n", name);
#endif

				/* Dump the monster attr/char info */
				auto_dump_printf("V:%d:0x%02X:0x%02X:0x%02X:0x%02X\n\n",
					i, kv, rv, gv, bv);
			}

			/* Close */
			close_auto_dump();

			/* Message */
#ifdef JP
			msg_print("カラーの設定をファイルに書き出しました。");
#else
			msg_print("Dumped color redefinitions.");
#endif

		}

		/* Edit colors */
		else if (i == '3')
		{
			static byte a = 0;

			/* Prompt */
#ifdef JP
			prt("コマンド: カラーの設定を変更します", 8, 0);
#else
			prt("Command: Modify colors", 8, 0);
#endif


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
#ifdef JP
				name = ((a < 16) ? color_names[a] : "未定義");
#else
				name = ((a < 16) ? color_names[a] : "undefined");
#endif


				/* Describe the color */
#ifdef JP
				Term_putstr(5, 10, -1, TERM_WHITE,
					    format("カラー = %d, 名前 = %s", a, name));
#else
				Term_putstr(5, 10, -1, TERM_WHITE,
					    format("Color = %d, Name = %s", a, name));
#endif


				/* Label the Current values */
				Term_putstr(5, 12, -1, TERM_WHITE,
					    format("K = 0x%02x / R,G,B = 0x%02x,0x%02x,0x%02x",
						   angband_color_table[a][0],
						   angband_color_table[a][1],
						   angband_color_table[a][2],
						   angband_color_table[a][3]));

				/* Prompt */
#ifdef JP
				Term_putstr(0, 14, -1, TERM_WHITE,
					    "コマンド (n/N/k/K/r/R/g/G/b/B): ");
#else
				Term_putstr(0, 14, -1, TERM_WHITE,
					    "Command (n/N/k/K/r/R/g/G/b/B): ");
#endif


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
#ifdef JP
	if (!get_string("メモ: ", buf, 60)) return;
#else
	if (!get_string("Note: ", buf, 60)) return;
#endif


	/* Ignore empty notes */
	if (!buf[0] || (buf[0] == ' ')) return;

	/* Add the note to the message recall */
#ifdef JP
	msg_format("メモ: %s", buf);
#else
	msg_format("Note: %s", buf);
#endif

}


/*
 * Mention the current version
 */
void do_cmd_version(void)
{

	/* Silly message */
#ifdef JP
	msg_format("変愚蛮怒(Hengband) %d.%d.%d",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
	msg_format("You are playing Hengband %d.%d.%d.",
		    FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif
}



/*
 * Array of feeling strings
 */
static cptr do_cmd_feeling_text[11] =
{
#ifdef JP
	"この階の雰囲気を感じとれなかった...",
#else
	"Looks like any other level.",
#endif

#ifdef JP
	"この階には何か特別なものがあるような気がする。",
#else
	"You feel there is something special about this level.",
#endif

#ifdef JP
	"恐ろしい死の幻が目に浮かび、気絶しそうになった！",
#else
	"You nearly faint as horrible visions of death fill your mind!",
#endif

#ifdef JP
	"この階はとても危険なようだ。",
#else
	"This level looks very dangerous.",
#endif

#ifdef JP
	"とても悪い予感がする...",
#else
	"You have a very bad feeling...",
#endif

#ifdef JP
	"悪い予感がする...",
#else
	"You have a bad feeling...",
#endif

#ifdef JP
	"何か緊張する。",
#else
	"You feel nervous.",
#endif

#ifdef JP
	"少し不運な気がする...",
#else
	"You feel your luck is turning...",
#endif

#ifdef JP
	"この場所は好きになれない。",
#else
	"You don't like the look of this place.",
#endif

#ifdef JP
	"この階はそれなりに安全なようだ。",
#else
	"This level looks reasonably safe.",
#endif

#ifdef JP
	"なんて退屈なところだ..."
#else
	"What a boring place..."
#endif

};

static cptr do_cmd_feeling_text_combat[11] =
{
#ifdef JP
	"この階の雰囲気を感じとれなかった...",
#else
	"Looks like any other level.",
#endif

#ifdef JP
	"この階には何か特別なものがあるような気がする。",
#else
	"You feel there is something special about this level.",
#endif

#ifdef JP
	"今夜もまた、誰かが命を落とす...",
#else
	"You nearly faint as horrible visions of death fill your mind!",
#endif

#ifdef JP
	"この階はとても危険なようだ。",
#else
	"This level looks very dangerous.",
#endif

#ifdef JP
	"とても悪い予感がする...",
#else
	"You have a very bad feeling...",
#endif

#ifdef JP
	"悪い予感がする...",
#else
	"You have a bad feeling...",
#endif

#ifdef JP
	"何か緊張する。",
#else
	"You feel nervous.",
#endif

#ifdef JP
	"少し不運な気がする...",
#else
	"You feel your luck is turning...",
#endif

#ifdef JP
	"この場所は好きになれない。",
#else
	"You don't like the look of this place.",
#endif

#ifdef JP
	"この階はそれなりに安全なようだ。",
#else
	"This level looks reasonably safe.",
#endif

#ifdef JP
	"なんて退屈なところだ..."
#else
	"What a boring place..."
#endif

};

static cptr do_cmd_feeling_text_lucky[11] =
{
#ifdef JP
	"この階の雰囲気を感じとれなかった...",
	"この階には何か特別なものがあるような気がする。",
	"この階はこの上なく素晴らしい感じがする。",
	"素晴らしい感じがする...",
	"とても良い感じがする...",
	"良い感じがする...",
	"ちょっと幸運な感じがする...",
	"多少は運が向いてきたか...",
	"見た感じ悪くはない...",
	"全然駄目ということはないが...",
	"なんて退屈なところだ..."
#else
	"Looks like any other level.",
	"You feel there is something special about this level.",
	"You have a superb feeling about this level.",
	"You have an excellent feeling...",
	"You have a very good feeling...",
	"You have a good feeling...",
	"You feel strangely lucky...",
	"You feel your luck is turning...",
	"You like the look of this place...",
	"This level can't be all bad...",
	"What a boring place..."
#endif
};


/*
 * Note that "feeling" is set to zero unless some time has passed.
 * Note that this is done when the level is GENERATED, not entered.
 */
void do_cmd_feeling(void)
{
	/* Verify the feeling */
	if (feeling > 10) feeling = 10;

	/* No useful feeling in quests */
	if (p_ptr->inside_quest && !random_quest_number(dun_level))
	{
#ifdef JP
		msg_print("典型的なクエストのダンジョンのようだ。");
#else
		msg_print("Looks like a typical quest level.");
#endif

		return;
	}

	/* No useful feeling in town */
	else if (p_ptr->town_num && !dun_level)
	{
#ifdef JP
		if (!strcmp(town[p_ptr->town_num].name, "荒野"))
#else
		if (!strcmp(town[p_ptr->town_num].name, "wilderness"))
#endif
		{
#ifdef JP
			msg_print("何かありそうな荒野のようだ。");
#else
			msg_print("Looks like a strange wilderness.");
#endif

			return;
		}
		else
		{
#ifdef JP
			msg_print("典型的な町のようだ。");
#else
			msg_print("Looks like a typical town.");
#endif

			return;
		}
	}

	/* No useful feeling in the wilderness */
	else if (!dun_level)
	{
#ifdef JP
		msg_print("典型的な荒野のようだ。");
#else
		msg_print("Looks like a typical wilderness.");
#endif

		return;
	}

	/* Display the feeling */
	if (turn - old_turn >= (150 - dun_level)*TURNS_PER_TICK || cheat_xtra)
	{
		if (p_ptr->muta3 & MUT3_GOOD_LUCK) msg_print(do_cmd_feeling_text_lucky[feeling]);
		else {
					if((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON)){
						msg_print(do_cmd_feeling_text_combat[feeling]);
					}else
						msg_print(do_cmd_feeling_text[feeling]);
				}
	}
	else
	{
		msg_print(do_cmd_feeling_text[0]);
	}
}



/*
 * Description of each monster group.
 */
static cptr monster_group_text[] = 
{
#ifdef JP
	"ユニーク",	/* "Uniques" */
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
#else
	"Uniques",
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
	"$!?=&`.|/\\~[]()>",
	"#",
	",",
	"*",
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
 * mode & 0x02 : cheat?
 */
static int collect_monsters(int grp_cur, s16b mon_idx[], byte mode)
{
	int i, mon_cnt = 0;
	int dummy_why;

	/* Get a list of x_char in this group */
	cptr group_char = monster_group_char[grp_cur];

	/* XXX Hack -- Check if this is the "Uniques" group */
	bool grp_unique = (monster_group_char[grp_cur] == (char *) -1L);

	/* Check every race */
	for (i = 0; i < max_r_idx; i++)
	{
		/* Access the race */
		monster_race *r_ptr = &r_info[i];

		/* Skip empty race */
		if (!r_ptr->name) continue ;

		/* Require known monsters */
		if (!(mode & 0x02) && !cheat_know && !r_ptr->r_sights ) continue;

		if (grp_unique && !(r_ptr->flags1 & RF1_UNIQUE)) continue;

		/* Check for race in the group */
		if (grp_unique || strchr(group_char, r_ptr->d_char))
		{
			/* Add the race */
			mon_idx[mon_cnt++] = i;

			/* XXX Hack -- Just checking for non-empty group */
			if (mode & 0x01) break;
		}
	}

	/* Terminate the list */
	mon_idx[mon_cnt] = 0;

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
	"Parchements",
	"Spikes",
	"Boxs",
	"Figurines",
	"Statues",
	"Junks",
	"Bottles",
	"Skeletons",
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
	TV_PARCHEMENT,
	TV_SPIKE,
	TV_CHEST,
	TV_FIGURINE,
	TV_STATUE,
	TV_JUNK,
	TV_BOTTLE,
	TV_SKELETON,
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
	0
};


/*
 * Build a list of monster indexes in the given group. Return the number
 * of monsters in the group.
 */
static int collect_objects(int grp_cur, int object_idx[])
{
	int i, j, k, object_cnt = 0;

	/* Get a list of x_char in this group */
	byte group_tval = object_group_tval[grp_cur];

	/* Check every object */
	for (i = 0; i < max_k_idx; i++)
	{
		/* Access the race */
		object_kind *k_ptr = &k_info[i];

		/* Skip empty objects */
		if (!k_ptr->name) continue;

		/* Skip non-flavoured objects */
		if (!k_ptr->flavor && !p_ptr->wizard) continue;

		/* Skip items with no distribution (special artifacts) */
		for (j = 0, k = 0; j < 4; j++) k += k_ptr->chance[j];
		if (!(k))  continue; 

		/* Require objects ever seen*/
		if (!k_ptr->aware && !p_ptr->wizard) continue;

		/* Check for race in the group */
		if (TV_LIFE_BOOK == group_tval)
		{
			/* Hack -- All spell books */
			if (TV_LIFE_BOOK <= k_ptr->tval && k_ptr->tval <= TV_HISSATSU_BOOK)
			{
				/* Add the race */
				object_idx[object_cnt++] = i;
			}
		}
		else if (k_ptr->tval == group_tval)
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
 */
static int collect_features(int grp_cur, int *feat_idx)
{
	int i, feat_cnt = 0;

	/* Unused;  There is a single group. */
	(void)grp_cur;

	/* Check every feature */
	for (i = 1; i < max_f_idx; i++)
	{
		/* Access the index */
		feature_type *f_ptr = &f_info[i];

		/* Skip empty index */
		if (!f_ptr->name) continue;

		/* Skip mimiccing features */
		if (f_ptr->mimic != i) continue;

		/* Add the index */
		feat_idx[feat_cnt++] = i;
	}

	/* Terminate the list */
	feat_idx[feat_cnt] = 0;

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


static errr photo_fgets(FILE *fff, char *buf, huge n)
{
	huge i = 0;

	char *s;

	char tmp[1024];

	/* Read a line */
	if (fgets(tmp, 1024, fff))
	{
		/* Convert weirdness */
		for (s = tmp; *s; s++)
		{
			/* Handle newline */
			if (*s == '\n')
			{
				/* Terminate */
				buf[i] = '\0';

				/* Success */
				return (0);
			}

			/* Handle tabs */
			else if (*s == '\t')
			{
				/* Hack -- require room */
				if (i + 8 >= n) break;

				/* Append a space */
				buf[i++] = ' ';

				/* Append some more spaces */
				while (!(i % 8)) buf[i++] = ' ';
			}

#ifdef JP
			else if (iskanji(*s))
			{
				if (!s[1]) break;
				buf[i++] = *s++;
				buf[i++] = *s;
			}
# ifndef EUC
	/* 半角かなに対応 */
			else if ((((int)*s & 0xff) > 0xa1) && (((int)*s & 0xff ) < 0xdf))
			{
				buf[i++] = *s;
				if (i >= n) break;
			}
# endif
#endif
			/* Handle printables */
			else
			{
				/* Copy */
				buf[i++] = *s;

				/* Check length */
				if (i >= n) break;
			}
		}
	}

	/* Nothing */
	buf[0] = '\0';

	/* Failure */
	return (1);
}


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
#ifdef JP
		msg_format("%s を開くことができませんでした。", buf);
#else
		msg_format("Failed to open %s.", buf);
#endif
		msg_print(NULL);
		return;
	}


	/* Save the screen */
	screen_save();

	/* Clear the screen */
	Term_clear();


	/* Load the screen */
	for (y = 0; okay && (y < hgt); y++)
	{
		/* Get a line of data */
		if (photo_fgets(fff, buf, 1024)) okay = FALSE;

		/* Show each row */
		for (x = 0; x < wid - 1; x++)
		{
			/* Put the attr/char */
			Term_draw(x, y, TERM_WHITE, buf[x]);
		}
	}

	/* Get the blank line */
	if (my_fgets(fff, buf, sizeof(buf))) okay = FALSE;


	/* Dump the screen */
	for (y = 0; okay && (y < hgt); y++)
	{
		/* Get a line of data */
		if (photo_fgets(fff, buf, 1024)) okay = FALSE;

		/* Dump each row */
		for (x = 0; x < wid - 1; x++)
		{
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


	/* Get the blank line */
	if (my_fgets(fff, buf, sizeof(buf))) okay = FALSE;


	/* Close it */
	my_fclose(fff);


	/* Message */
#ifdef JP
	prt("ファイルに書き出された画面(記念撮影)をロードしました。", 0, 0);
#else
	msg_print("Screen dump loaded.");
#endif

	flush();
	inkey();


	/* Restore the screen */
	screen_load();
}




cptr inven_res_label = 
#ifdef JP
 "                               酸電火冷毒光闇破轟獄因沌劣 盲怖乱痺透命感消復浮";
#else
 "                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSeHlEpSdRgLv";
#endif

/* XTRA HACK RESLIST */
static void do_cmd_knowledge_inven_aux(FILE *fff, object_type *o_ptr, 
				       int *j, byte tval, char *where)
{
  char o_name[MAX_NLEN];
  u32b flgs[TR_FLAG_SIZE];

  if (!o_ptr->k_idx)return;
  if (o_ptr->tval != tval)return;

       /* 
	* HACK:Ring of Lordly protection and Dragon shield/helm
	* have random resistances.
	*/
  if ( ((o_ptr->tval >= TV_BOW && o_ptr->tval<= TV_DRAG_ARMOR && o_ptr->name2)
       || (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_LORDLY) 
       || (o_ptr->tval == TV_SHIELD && o_ptr->sval == SV_DRAGON_SHIELD) 
       || (o_ptr->tval == TV_HELM && o_ptr->sval == SV_DRAGON_HELM) 
       || (o_ptr->tval == TV_GLOVES && o_ptr->sval == SV_SET_OF_DRAGON_GLOVES) 
       || (o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) 
       || o_ptr->art_name || o_ptr->name1) && object_known_p(o_ptr))
    {
      int i = 0;
      object_desc(o_name, o_ptr, TRUE, 0);

      while ( o_name[i] && i < 26 ){
#ifdef JP
	if (iskanji(o_name[i])) i++;
#endif
	i++;
      }
      if(i<28) while(i<28){o_name[i]=' ';i++;}
      o_name[i]=0;
      
      fprintf(fff,"%s %s", where, o_name);

      if (!(o_ptr->ident & (IDENT_MENTAL))) 
	{
#ifdef JP
	  fprintf(fff, "-------不明--------------- -------不明---------\n");
#else
	  fprintf(fff, "-------unknown------------ -------unknown------\n");
#endif
	}
      else {
	object_flags_known(o_ptr, flgs);
      
#ifdef JP
	if (have_flag(flgs, TR_IM_ACID)) fprintf(fff,"＊");
	else if (have_flag(flgs, TR_RES_ACID)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_IM_ELEC)) fprintf(fff,"＊");
	else if (have_flag(flgs, TR_RES_ELEC)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_IM_FIRE)) fprintf(fff,"＊");
	else if (have_flag(flgs, TR_RES_FIRE)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_IM_COLD)) fprintf(fff,"＊");
	else if (have_flag(flgs, TR_RES_COLD)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_POIS)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_LITE)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_DARK)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_SHARDS)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_SOUND)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_NETHER)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_NEXUS)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_CHAOS)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_DISEN)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	fprintf(fff," ");
	
	if (have_flag(flgs, TR_RES_BLIND)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_FEAR)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_RES_CONF)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_FREE_ACT)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_SEE_INVIS)) fprintf(fff,"＋");
	else fprintf(fff,"・");
	
	if (have_flag(flgs, TR_HOLD_LIFE)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_TELEPATHY)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_SLOW_DIGEST)) fprintf(fff,"＋");
	else fprintf(fff,"・");


	if (have_flag(flgs, TR_REGEN)) fprintf(fff,"＋");
	else fprintf(fff,"・");

	if (have_flag(flgs, TR_FEATHER)) fprintf(fff,"＋");
	else fprintf(fff,"・");
#else
	if (have_flag(flgs, TR_IM_ACID)) fprintf(fff,"* ");
	else if (have_flag(flgs, TR_RES_ACID)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_IM_ELEC)) fprintf(fff,"* ");
	else if (have_flag(flgs, TR_RES_ELEC)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_IM_FIRE)) fprintf(fff,"* ");
	else if (have_flag(flgs, TR_RES_FIRE)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_IM_COLD)) fprintf(fff,"* ");
	else if (have_flag(flgs, TR_RES_COLD)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_POIS)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_LITE)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_DARK)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_SHARDS)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_SOUND)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_NETHER)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_NEXUS)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_CHAOS)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_DISEN)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	fprintf(fff," ");
	
	if (have_flag(flgs, TR_RES_BLIND)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_FEAR)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_RES_CONF)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_FREE_ACT)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_SEE_INVIS)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
	
	if (have_flag(flgs, TR_HOLD_LIFE)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_TELEPATHY)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_SLOW_DIGEST)) fprintf(fff,"+ ");
	else fprintf(fff,". ");


	if (have_flag(flgs, TR_REGEN)) fprintf(fff,"+ ");
	else fprintf(fff,". ");

	if (have_flag(flgs, TR_FEATHER)) fprintf(fff,"+ ");
	else fprintf(fff,". ");
#endif	
	fprintf(fff,"\n");
      }
      (*j)++;
      if(*j==9)
	{ 
	  *j=0;
	  fprintf(fff,"%s\n", inven_res_label);
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
	object_type *o_ptr;

	byte tval;
	int i=0;
	int j=0;

	char  where[32];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
	    msg_print(NULL);
	    return;
	}
	fprintf(fff,"%s\n",inven_res_label);

	for (tval=TV_BOW; tval <= TV_RING; tval++){

	  if (j!=0) {
	      for (;j<9;j++) fprintf(fff, "\n");
	      j=0;
	      fprintf(fff,"%s\n",inven_res_label);              
	  }
	  
#ifdef JP
	  strcpy(where, "装");
#else
	  strcpy(where, "E ");
#endif
	  for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	    {
	      o_ptr = &inventory[i];
	      do_cmd_knowledge_inven_aux(fff, o_ptr, &j, tval, where);
	    }
	  
#ifdef JP
	  strcpy(where, "持");
#else
	  strcpy(where, "I ");
#endif
	  for (i = 0; i < INVEN_PACK; i++)
	    {
	      o_ptr = &inventory[i];
	      do_cmd_knowledge_inven_aux(fff, o_ptr, &j, tval, where);
	    }
	  
	  
	  /* Print all homes in the different towns */
	  st_ptr = &town[1].store[STORE_HOME];
#ifdef JP
	  strcpy(where, "家");
#else
	  strcpy(where, "H ");
#endif
	      
	  /* Dump all available items */
	  for (i = 0; i < st_ptr->stock_num; i++)
	    {
	      o_ptr = &st_ptr->stock[i];
	      do_cmd_knowledge_inven_aux(fff, o_ptr, &j, tval, where);
	    }
	}
	  
	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "*鑑定*済み武器/防具の耐性リスト", 0, 0);
#else
	show_file(TRUE, file_name, "Resistances of *identified* equipment", 0, 0);
#endif

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
#ifdef JP
		    msg_format("ファイル %s を開けませんでした。", filename);
#else
		    msg_format("Failed to open file %s.", filename);
#endif
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
			fprintf(fff, html_head[i]);
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
			fprintf(fff, html_foot[i]);
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
#ifdef JP
	msg_print("画面(記念撮影)をファイルに書き出しました。");
#else
		msg_print("Screen dump saved.");
#endif
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

#ifdef JP
	if (!get_string("ファイル名: ", tmp, 80))
#else
	if (!get_string("File name: ", tmp, 80))
#endif
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

#ifdef JP
	prt("記念撮影しますか？ [(y)es/(h)tml/(n)o] ", 0, 0);
#else
	prt("Save screen dump? [(y)es/(h)tml/(n)o] ", 0, 0);
#endif
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
#ifdef JP
			msg_format("ファイル %s を開けませんでした。", buf);
#else
			msg_format("Failed to open file %s.", buf);
#endif
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
#ifdef JP
	msg_print("画面(記念撮影)をファイルに書き出しました。");
#else
		msg_print("Screen dump saved.");
#endif

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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
				if (!artifact_p(o_ptr)) continue;

				/* Ignore known items */
				if (object_known_p(o_ptr)) continue;

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
		if (!artifact_p(o_ptr)) continue;

		/* Ignore known items */
		if (object_known_p(o_ptr)) continue;

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
#ifdef JP
		strcpy(base_name, "未知の伝説のアイテム");
#else
		strcpy(base_name, "Unknown Artifact");
#endif


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

			/* Describe the artifact */
			object_desc_store(base_name, q_ptr, FALSE, 0);
		}

		/* Hack -- Build the artifact name */
#ifdef JP
		fprintf(fff, "     %s\n", base_name);
#else
		fprintf(fff, "     The %s\n", base_name);
#endif

	}

	/* Free the "who" array */
	C_KILL(who, max_a_idx, s16b);

	/* Free the "okay" array */
	C_KILL(okay, max_a_idx, bool);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "既知の伝説のアイテム", 0, 0);
#else
	show_file(TRUE, file_name, "Artifacts Seen", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display known uniques
 */
static void do_cmd_knowledge_uniques(void)
{
	int i, k, n = 0;
	u16b why = 2;
	s16b *who;

	FILE *fff;

	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);

	if (!fff) {
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
	    msg_print(NULL);
	    return;
	}

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

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

		/* Only print Uniques */
		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			bool dead = (r_ptr->max_num == 0);

			if (dead) continue;

			/* Only display "known" uniques */
			if (dead || cheat_know || r_ptr->r_sights)
			{
				/* Print a message */
#ifdef JP
				fprintf(fff, "     %sはまだ生きている。\n",
					(r_name + r_ptr->name));
#else
				fprintf(fff, "     %s is alive\n",
					(r_name + r_ptr->name));
#endif

			}
		}
	}

	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "まだ生きているユニーク・モンスター", 0, 0);
#else
	show_file(TRUE, file_name, "Alive Uniques", 0, 0);
#endif


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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
	show_file(TRUE, file_name, "武器の経験値", 0, 0);
#else
	show_file(TRUE, file_name, "Weapon Proficiency", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display spell-exp
 */
static void do_cmd_knowledge_spell_exp(void)
{
	int i = 0, spell_exp, exp_level;

	FILE *fff;
	magic_type *s_ptr;

	char file_name[1024];

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
	    msg_print(NULL);
	    return;
	}

	if (p_ptr->realm1 != REALM_NONE)
	{
#ifdef JP
		fprintf(fff, "%sの魔法書\n", realm_names[p_ptr->realm1]);
#else
		fprintf(fff, "%s Spellbook\n", realm_names[p_ptr->realm1]);
#endif
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
			fprintf(fff, "%-25s ", spell_names[technic2magic(p_ptr->realm1) - 1][i]);
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
#ifdef JP
		fprintf(fff, "%sの魔法書\n", realm_names[p_ptr->realm2]);
#else
		fprintf(fff, "\n%s Spellbook\n", realm_names[p_ptr->realm2]);
#endif
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
			fprintf(fff, "%-25s ", spell_names[technic2magic(p_ptr->realm2) - 1][i]);
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
#ifdef JP
	show_file(TRUE, file_name, "魔法の経験値", 0, 0);
#else
	show_file(TRUE, file_name, "Spell Proficiency", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display skill-exp
 */
static void do_cmd_knowledge_skill_exp(void)
{
	int i = 0, skill_exp;

	FILE *fff;

	char file_name[1024];
#ifdef JP
	char skill_name[3][17]={"マーシャルアーツ", "二刀流          ", "乗馬            "};
#else
	char skill_name[3][20]={"Martial Arts    ", "Dual Wielding   ", "Riding          "};
#endif

	/* Open a new file */
	fff = my_fopen_temp(file_name, 1024);
	if (!fff) {
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
	show_file(TRUE, file_name, "技能の経験値", 0, 0);
#else
	show_file(TRUE, file_name, "Miscellaneous Proficiency", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Pluralize a monster name
 */
void plural_aux(char *Name)
{
	int NameLen = strlen(Name);

	if (strstr(Name, "Disembodied hand"))
	{
		strcpy(Name, "Disembodied hands that strangled people");
	}
	else if (strstr(Name, "Colour out of space"))
	{
		strcpy(Name, "Colours out of space");
	}
	else if (strstr(Name, "stairway to hell"))
	{
		strcpy(Name, "stairways to hell");
	}
	else if (strstr(Name, "Dweller on the threshold"))
	{
		strcpy(Name, "Dwellers on the threshold");
	}
	else if (strstr(Name, " of "))
	{
		cptr aider = strstr(Name, " of ");
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
	else if (strstr(Name, "coins"))
	{
		char dummy[80];
		strcpy(dummy, "piles of ");
		strcat(dummy, Name);
		strcpy(Name, dummy);
		return;
	}
	else if (strstr(Name, "Manes"))
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

/*
 * Display current pets
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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

	/* Process the waiting pets (backwards) */
	for (i = MAX_PARTY_MON - 1; i >= 0; i--)
	{
		/* Access the monster */
		m_ptr = &party_mon[i];

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		t_friends++;

		/* List waiting pets */
		monster_desc(pet_name, m_ptr, MD_ASSUME_OUTSIDE | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
		fprintf(fff, "%s (%s)\n", pet_name, look_mon_desc(m_ptr, 0x00));
	}

	show_upkeep = calculate_upkeep();

	fprintf(fff, "----------------------------------------------\n");
#ifdef JP
	fprintf(fff, "    合計: %d 匹のペット\n", t_friends);
	fprintf(fff, " 維持コスト: %d%% MP\n", show_upkeep);
#else
	fprintf(fff, "   Total: %d pet%s.\n",
		t_friends, (t_friends == 1 ? "" : "s"));
	fprintf(fff, "   Upkeep: %d%% mana.\n", show_upkeep);
#endif



	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "現在のペット", 0, 0);
#else
	show_file(TRUE, file_name, "Current Pets", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Total kill count
 *
 * Note that the player ghosts are ignored.  XXX XXX XXX
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
			fprintf(fff,"あなたはまだ敵を倒していない。\n\n");
#else
			fprintf(fff,"You have defeated no enemies yet.\n\n");
#endif

		else if (Total == 1)
#ifdef JP
			fprintf(fff,"あなたは一匹の敵を倒している。\n\n");
#else
			fprintf(fff,"You have defeated one enemy.\n\n");
#endif

		else
#ifdef JP
			fprintf(fff,"あなたは %lu 匹の敵を倒している。\n\n", Total);
#else
			fprintf(fff,"You have defeated %lu enemies.\n\n", Total);
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
				/* p,tは人と数える by ita*/
				if(strchr("pt",r_ptr->d_char))
					fprintf(fff, "     %3d 人の %s\n", This, r_name + r_ptr->name);
				else
					fprintf(fff, "     %3d 匹の %s\n", This, r_name + r_ptr->name);
#else
				if (This < 2)
				{
					if (strstr(r_name + r_ptr->name, "coins"))
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
	fprintf(fff,"    合計: %lu 匹を倒した。\n", Total);
#else
	fprintf(fff,"   Total: %lu creature%s killed.\n",
		Total, (Total == 1 ? "" : "s"));
#endif


	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);

	/* Close the file */
	my_fclose(fff);

	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "倒した敵の数", 0, 0);
#else
	show_file(TRUE, file_name, "Kill Count", 0, 0);
#endif


	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Display the object groups.
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
			byte a, a2;
			char c, c2;
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

			if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

			/* Display symbol */
			Term_putch(x, y, a, c);

			/* Second byte */
			if (use_bigtile) Term_putch(x + 1, y, a2, c2);
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

/*
 *  Do visual mode command -- Change symbols
 */
static bool visual_mode_command(char ch, bool *visual_list_ptr, 
				int height, int width, 
				byte *attr_top_ptr, byte *char_left_ptr, 
				byte *cur_attr_ptr, byte *cur_char_ptr)
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
		/* Set the visual */
		attr_idx = *cur_attr_ptr;
		char_idx = *cur_char_ptr;

		return TRUE;

	case 'P':
	case 'p':
		if (attr_idx)
		{
			/* Set the char */
			*cur_attr_ptr = attr_idx;
			*attr_top_ptr = MAX(0, (*cur_attr_ptr & 0x7f) - 5);
		}

		if (char_idx)
		{
			/* Set the char */
			*cur_char_ptr = char_idx;
			*char_left_ptr = MAX(0, *cur_char_ptr - 10);
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
	int mon_cur, int mon_top)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && mon_idx[mon_top + i]; i++)
	{
		byte attr;
		byte a, a2;
		char c, c2;

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
			c_prt(attr, format("%02x/%02x", r_ptr->x_attr, r_ptr->x_char), row + i, 60);
		}
		else if (p_ptr->wizard) 
		{
			c_prt(attr, format("%d", r_idx), row + i, 60);
		}

		a = r_ptr->x_attr;
		c = r_ptr->x_char;
		if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

		/* Display symbol */
		Term_putch(70, row + i, a, c);

		/* Second byte */
		if (use_bigtile) Term_putch(70 + 1, row + i, a2, c2);

		/* Display kills */
		if (!(r_ptr->flags1 & RF1_UNIQUE)) put_str(format("%5d", r_ptr->r_pkills), row + i, 73);
#ifdef JP
		else c_put_str((r_ptr->max_num == 0 ? TERM_L_DARK : TERM_WHITE), (r_ptr->max_num == 0 ? "死亡" : "生存"), row + i, 73);
#else
		else c_put_str((r_ptr->max_num == 0 ? TERM_L_DARK : TERM_WHITE), (r_ptr->max_num == 0 ? "dead" : "alive"), row + i, 73);
#endif
	
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
static void do_cmd_knowledge_monsters(void)
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

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "mon_idx" array */
	C_MAKE(mon_idx, max_r_idx, s16b);

	max = 0;
	grp_cnt = 0;

	/* Check every group */
	for (i = 0; monster_group_text[i] != NULL; i++)
	{
		/* Measure the label */
		len = strlen(monster_group_text[i]);

		/* Save the maximum length */
		if (len > max) max = len;

		/* See if any monsters are known */
		if ((monster_group_char[i] == ((char *) -1L)) || collect_monsters(i, mon_idx, 0x01))
		{
			/* Build a list of groups with known monsters */
			grp_idx[grp_cnt++] = i;
		}
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	mon_cur = mon_top = 0;
	mon_cnt = 0;

	flag = FALSE;
	redraw = TRUE;

	while (!flag)
	{
		char ch;
		monster_race *r_ptr;

		if (redraw)
		{
			clear_from(0);
		
#ifdef JP
			prt("知識 - モンスター", 2, 0);
			prt("グループ", 4, 0);
			prt("名前", 4, max + 3);
			if (p_ptr->wizard) prt("Idx", 4, 60);
			prt("文字 殺害数", 4, 67);
#else
			prt("Knowledge - Monsters", 2, 0);
			prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			if (p_ptr->wizard) prt("Idx", 4, 60);
			prt("Sym   Kills", 4, 67);
#endif

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			for (i = 0; i < browser_rows; i++)
			{
				Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
			}

			redraw = FALSE;
		}

		/* Scroll group list */
		if (grp_cur < grp_top) grp_top = grp_cur;
		if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

		/* Display a list of monster groups */
		display_group_list(0, 6, max, browser_rows, grp_idx, monster_group_text, grp_cur, grp_top);

		if (old_grp_cur != grp_cur)
		{
			old_grp_cur = grp_cur;

			/* Get a list of monsters in the current group */
			mon_cnt = collect_monsters(grp_idx[grp_cur], mon_idx, 0x00);
		}

		/* Scroll monster list */
		while (mon_cur < mon_top)
			mon_top = MAX(0, mon_top - browser_rows/2);
		while (mon_cur >= mon_top + browser_rows)
			mon_top = MIN(mon_cnt - browser_rows, mon_top + browser_rows/2);

		if (!visual_list)
		{
			/* Display a list of monsters in the current group */
			display_monster_list(max + 3, 6, browser_rows, mon_idx, mon_cur, mon_top);
		}
		else
		{
			mon_top = mon_cur;

			/* Display a monster name */
			display_monster_list(max + 3, 6, 1, mon_idx, mon_cur, mon_top);

			/* Display visual list below first monster */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Prompt */
#ifdef JP
		prt(format("<方向>, 'r'で思い出を見る%s%s, ESC", visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更", (attr_idx||char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"), hgt - 1, 0);
#else
		prt(format("<dir>, 'r' to recall%s%s, ESC", visual_list ? ", ENTER to accept" : ", 'v' for visuals", (attr_idx||char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"), hgt - 1, 0);
#endif

		/* Get the current monster */
		r_ptr = &r_info[mon_idx[mon_cur]];

		/* Mega Hack -- track this monster race */
		if (mon_cnt) monster_race_track(mon_idx[mon_cur]);

		/* Hack -- handle stuff */
		handle_stuff();

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
		if (visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, &r_ptr->x_attr, &r_ptr->x_char)) continue;

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
				if (mon_idx[mon_cur])
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
	int object_cur, int object_top)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && object_idx[object_top + i]; i++)
	{
		char o_name[80];
		byte a, a2;
		char c, c2;

		/* Get the object index */
		int k_idx = object_idx[object_top + i];

		/* Access the object */
		object_kind *k_ptr = &k_info[k_idx];

		/* Choose a color */
		byte attr = (k_ptr->aware ? TERM_WHITE : TERM_SLATE);
		byte cursor = (k_ptr->aware ? TERM_L_BLUE : TERM_BLUE);

		attr = ((i + object_top == object_cur) ? cursor : attr);
		
		/* Tidy name */
		strip_name(o_name, k_idx);

		/* Display the name */
		c_prt(attr, o_name, row + i, col);

		/* Hack -- visual_list mode */
		if (per_page == 1)
		{
			c_prt(attr, format("%02x/%02x", k_ptr->x_attr, k_ptr->x_char), row + i, 60);
		}
		else if (p_ptr->wizard)
		{
			c_prt(attr, format ("%d", k_idx), row + i, 70);
		}

		a = k_ptr->flavor ? misc_to_attr[k_ptr->flavor] : k_ptr->x_attr;
		c = k_ptr->flavor ? misc_to_char[k_ptr->flavor] : k_ptr->x_char;

		/* Symbol is unknown */	
		if (!k_ptr->aware && !p_ptr->wizard)
		{
			c = ' ';
			a = TERM_DARK;
		}

		if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

		/* Display symbol */
		Term_putch(76, row + i, a, c);

		/* Second byte */
		if (use_bigtile) Term_putch(76 + 1, row + i, a2, c2);
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

	if (!screen_object(o_ptr, FALSE))
	{
#ifdef JP
		msg_print("特に変わったところはないようだ。");
#else
		msg_print("You see nothing special.");
#endif
		msg_print(NULL);
	}
}



/*
 * Display known objects
 */
static void do_cmd_knowledge_objects(void)
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

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "object_idx" array */
	C_MAKE(object_idx, max_k_idx, int);

	max = 0;
	grp_cnt = 0;

	/* Check every group */
	for (i = 0; object_group_text[i] != NULL; i++)
	{
		/* Measure the label */
		len = strlen(object_group_text[i]);

		/* Save the maximum length */
		if (len > max) max = len;

		/* See if any monsters are known */
		if (collect_objects(i, object_idx))
		{
			/* Build a list of groups with known monsters */
			grp_idx[grp_cnt++] = i;
		}
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	object_cur = object_top = 0;
	object_old = -1;
	object_cnt = 0;

	flag = FALSE;
	redraw = TRUE;

	while (!flag)
	{
		char ch;
		object_kind *k_ptr;

		if (redraw)
		{
			clear_from(0);
		
#ifdef JP
			prt("知識 - アイテム", 2, 0);
			prt("グループ", 4, 0);
			prt("名前", 4, max + 3);
			if (p_ptr->wizard) prt("Idx", 4, 70);
			prt("文字", 4, 75);
#else
			prt("Knowledge - objects", 2, 0);
			prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			if (p_ptr->wizard) prt("Idx", 4, 70);
			prt("Sym", 4, 75);
#endif

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			for (i = 0; i < browser_rows; i++)
			{
				Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
			}

			redraw = FALSE;
		}

		/* Scroll group list */
		if (grp_cur < grp_top) grp_top = grp_cur;
		if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

		/* Display a list of object groups */
		display_group_list(0, 6, max, browser_rows, grp_idx, object_group_text, grp_cur, grp_top);

		if (old_grp_cur != grp_cur)
		{
			old_grp_cur = grp_cur;

			/* Get a list of objects in the current group */
			object_cnt = collect_objects(grp_idx[grp_cur], object_idx);
		}

		/* Scroll object list */
		while (object_cur < object_top)
			object_top = MAX(0, object_top - browser_rows/2);
		while (object_cur >= object_top + browser_rows)
			object_top = MIN(object_cnt - browser_rows, object_top + browser_rows/2);

		if (!visual_list)
		{
			/* Display a list of objects in the current group */
			display_object_list(max + 3, 6, browser_rows, object_idx, object_cur, object_top);
		}
		else
		{
			object_top = object_cur;

			/* Display a list of objects in the current group */
			display_object_list(max + 3, 6, 1, object_idx, object_cur, object_top);

			/* Display visual list below first object */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Get the current object */
		k_ptr = &k_info[object_idx[object_cur]];

		/* Mega Hack -- track this object */
		if (object_cnt) object_kind_track(object_idx[object_cur]);

		/* Prompt */
#ifdef JP
		prt(format("<方向>, 'r'で思い出を見る%s%s, ESC", k_ptr->flavor ? "" : visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更", (attr_idx||char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"), hgt - 1, 0);
#else
		prt(format("<dir>, 'r' to recall%s%s, ESC", k_ptr->flavor ? "" : visual_list ? ", ENTER to accept" : ", 'v' for visuals", (attr_idx||char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"), hgt - 1, 0);
#endif

		/* The "current" object changed */
		if (object_old != object_idx[object_cur])
		{
			/* Hack -- handle stuff */
			handle_stuff();

			/* Remember the "current" object */
			object_old = object_idx[object_cur];
		}

		if (visual_list)
		{
			place_visual_list_cursor(max + 3, 7, k_ptr->x_attr, k_ptr->x_char, attr_top, char_left);
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
		/* Symbol of objects with flavor cannot be changed */
		if (!k_ptr->flavor && visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, &k_ptr->x_attr, &k_ptr->x_char)) continue;

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
				if (grp_cnt > 0)
					desc_obj_fake(object_idx[object_cur]);

				redraw = TRUE;
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
	int feat_cur, int feat_top)
{
	int i;

	/* Display lines until done */
	for (i = 0; i < per_page && feat_idx[feat_top + i]; i++)
	{
		byte a, a2;
		char c, c2;
		byte attr;

		/* Get the index */
		int f_idx = feat_idx[feat_top + i];

		/* Access the index */
		feature_type *f_ptr = &f_info[f_idx];

		/* Choose a color */
		attr = ((i + feat_top == feat_cur) ? TERM_L_BLUE : TERM_WHITE);

		/* Display the name */
		c_prt(attr, f_name + f_ptr->name, row + i, col);

		/* Hack -- visual_list mode */
		if (per_page == 1)
		{
			c_prt(attr, format("%02x/%02x", f_ptr->x_attr, f_ptr->x_char), row + i, 60);
		}

		a = f_ptr->x_attr;
		c = f_ptr->x_char;

		if (use_bigtile) bigtile_attr(&c, &a, &c2, &a2);

		/* Display symbol */
		Term_putch(68, row + i, a, c);

		/* Second byte */
		if (use_bigtile) Term_putch(68 + 1, row + i, a2, c2);
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
static void do_cmd_knowledge_features(void)
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

	/* Get size */
	Term_get_size(&wid, &hgt);

	browser_rows = hgt - 8;

	/* Allocate the "feat_idx" array */
	C_MAKE(feat_idx, max_f_idx, int);

	max = 0;
	grp_cnt = 0;

	/* Check every group */
	for (i = 0; feature_group_text[i] != NULL; i++)
	{
		/* Measure the label */
		len = strlen(feature_group_text[i]);

		/* Save the maximum length */
		if (len > max) max = len;

		/* See if any features are known */
		if (collect_features(i, feat_idx))
		{
			/* Build a list of groups with known features */
			grp_idx[grp_cnt++] = i;
		}
	}

	/* Terminate the list */
	grp_idx[grp_cnt] = -1;

	old_grp_cur = -1;
	grp_cur = grp_top = 0;
	feat_cur = feat_top = 0;
	feat_cnt = 0;

	flag = FALSE;
	redraw = TRUE;

	while ((!flag) && (grp_cnt))
	{
		char ch;
		feature_type *f_ptr;

		if (redraw)
		{
			clear_from(0);
		
			prt("Visuals - features", 2, 0);
			prt("Group", 4, 0);
			prt("Name", 4, max + 3);
			prt("Sym", 4, 67);

			for (i = 0; i < 78; i++)
			{
				Term_putch(i, 5, TERM_WHITE, '=');
			}

			for (i = 0; i < browser_rows; i++)
			{
				Term_putch(max + 1, 6 + i, TERM_WHITE, '|');
			}

			redraw = FALSE;
		}

		/* Scroll group list */
		if (grp_cur < grp_top) grp_top = grp_cur;
		if (grp_cur >= grp_top + browser_rows) grp_top = grp_cur - browser_rows + 1;

		/* Display a list of feature groups */
		display_group_list(0, 6, max, browser_rows, grp_idx, feature_group_text, grp_cur, grp_top);

		if (old_grp_cur != grp_cur)
		{
			old_grp_cur = grp_cur;

			/* Get a list of features in the current group */
			feat_cnt = collect_features(grp_idx[grp_cur], feat_idx);
		}

		/* Scroll feature list */
		while (feat_cur < feat_top)
			feat_top = MAX(0, feat_top - browser_rows/2);
		while (feat_cur >= feat_top + browser_rows)
			feat_top = MIN(feat_cnt - browser_rows, feat_top + browser_rows/2);

		if (!visual_list)
		{
			/* Display a list of features in the current group */
			display_feature_list(max + 3, 6, browser_rows, feat_idx, feat_cur, feat_top);
		}
		else
		{
			feat_top = feat_cur;

			/* Display a list of features in the current group */
			display_feature_list(max + 3, 6, 1, feat_idx, feat_cur, feat_top);

			/* Display visual list below first object */
			display_visual_list(max + 3, 7, browser_rows-1, wid - (max + 3), attr_top, char_left);
		}

		/* Prompt */
#ifdef JP
		prt(format("<方向>%s%s, ESC", visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更", (attr_idx||char_idx) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"), hgt - 1, 0);
#else
		prt(format("<dir>%s%s, ESC", visual_list ? ", ENTER to accept" : ", 'v' for visuals", (attr_idx||char_idx) ? ", 'c', 'p' to paste" : ", 'c' to copy"), hgt - 1, 0);
#endif

		/* Get the current feature */
		f_ptr = &f_info[feat_idx[feat_cur]];

		if (visual_list)
		{
			place_visual_list_cursor(max + 3, 7, f_ptr->x_attr, f_ptr->x_char, attr_top, char_left);
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

		/* Do visual mode command if needed */
		if (visual_mode_command(ch, &visual_list, browser_rows-1, wid - (max + 3), &attr_top, &char_left, &f_ptr->x_attr, &f_ptr->x_char)) continue;

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

	/* Prompt */
	if (!grp_cnt) msg_print("No features known.");

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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
			fprintf(fff,"\n%s\n", "賞金首はもう残っていません。");
#else
			fprintf(fff,"\n%s\n", "There is no more wanted monster.");
#endif
		}
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "賞金首の一覧", 0, 0);
#else
	show_file(TRUE, file_name, "Wanted monsters", 0, 0);
#endif

	
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
	    msg_print(NULL);
	    return;
	}
	
	if (fff)
	{
#ifdef JP
		fprintf(fff, "現在の属性 : %s\n\n", your_alignment());
#else
		fprintf(fff, "Your alighnment : %s\n\n", your_alignment());
#endif
		dump_virtues(fff);
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "八つの徳", 0, 0);
#else
	show_file(TRUE, file_name, "Virtues", 0, 0);
#endif

	
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
			fprintf(fff,"%c%-12s :  %3d 階\n", seiha ? '!' : ' ', d_name + d_info[i].name, max_dlv[i]);
#else
			fprintf(fff,"%c%-16s :  level %3d\n", seiha ? '!' : ' ', d_name + d_info[i].name, max_dlv[i]);
#endif
		}
	}
	
	/* Close the file */
	my_fclose(fff);
	
	/* Display the file contents */
#ifdef JP
	show_file(TRUE, file_name, "今までに入ったダンジョン", 0, 0);
#else
	show_file(TRUE, file_name, "Dungeon", 0, 0);
#endif

	
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
	show_file(TRUE, file_name, "自分に関する情報", 0, 0);
#else
	show_file(TRUE, file_name, "HP-rate & Max stat", 0, 0);
#endif

	
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

#ifdef JP
	fprintf(fff, "《遂行中のクエスト》\n");
#else
	fprintf(fff, "< Current Quest >\n");
#endif

	for (i = 1; i < max_quests; i++)
	{
		if ((quest[i].status == QUEST_STATUS_TAKEN) || (quest[i].status == QUEST_STATUS_COMPLETED))
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

				if (quest[i].status == QUEST_STATUS_TAKEN)
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
#ifdef JP
							sprintf(note," - %sを倒す。",name);
#else
							sprintf(note," - kill %s.",name);
#endif
						break;

					case QUEST_TYPE_FIND_ARTIFACT:
						strcpy(name, a_name + a_info[quest[i].k_idx].name);
#ifdef JP
						sprintf(note," - %sを見つけ出す。", name);
#else
						sprintf(note," - Find out %s.", name);
#endif
						break;

					case QUEST_TYPE_FIND_EXIT:
#ifdef JP
						sprintf(note," - 探索する。");
#else
						sprintf(note," - Search.");
#endif
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
#ifdef JP
						sprintf(note," - 全てのモンスターを倒す。");
#else
						sprintf(note," - Kill all monsters.");
#endif
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

				fprintf(fff, tmp_str);

				if (quest[i].status == QUEST_STATUS_COMPLETED)
				{
#ifdef JP
					sprintf(tmp_str, "    クエスト達成 - まだ報酬を受けとってない。\n");
#else
					sprintf(tmp_str, "    Quest Completed - Unrewarded\n");
#endif
					fprintf(fff, tmp_str);
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
	if (rand_tmp_str[0]) fprintf(fff, rand_tmp_str);

#ifdef JP
	if (!total) fprintf(fff, "  なし\n");
#else
	if (!total) fprintf(fff, "  Nothing.\n");
#endif
}


/*
 * Print all finished quests
 */
void do_cmd_knowledge_quests_completed(FILE *fff, int quest_num[])
{
	char tmp_str[120];
	int i;
	int total = 0;

#ifdef JP
	fprintf(fff, "《達成したクエスト》\n");
#else
	fprintf(fff, "< Completed Quest >\n");
#endif
	for (i = 1; i < max_quests; i++)
	{
		int q_idx = quest_num[i];

		if (quest[q_idx].status == QUEST_STATUS_FINISHED)
		{
			if (is_fixed_quest_idx(q_idx))
			{
				/* Set the quest number temporary */
				int old_quest = p_ptr->inside_quest;

				p_ptr->inside_quest = q_idx;

				/* Get the quest */
				init_flags = INIT_ASSIGN;

				process_dungeon_file("q_info.txt", 0, 0, 0, 0);

				/* Reset the old quest number */
				p_ptr->inside_quest = old_quest;

				/* No info from "silent" quests */
				if (quest[q_idx].flags & QUEST_FLAG_SILENT) continue;
			}

			total++;

			if (!is_fixed_quest_idx(q_idx) && quest[q_idx].r_idx)
			{
				/* Print the quest info */

				if (quest[q_idx].complev == 0)
				{
					sprintf(tmp_str,
#ifdef JP
						"  %-40s (%2d階)            -   不戦勝\n",
#else
						"  %-40s (Dungeon level: %2d) - (Cancelled)\n",
#endif
						r_name+r_info[quest[q_idx].r_idx].name,
						quest[q_idx].level);
				}
				else
				{
					sprintf(tmp_str,
#ifdef JP
						"  %-40s (%2d階)            - レベル%2d\n",
#else
						"  %-40s (Dungeon level: %2d) - level %2d\n",
#endif
						r_name+r_info[quest[q_idx].r_idx].name,
						quest[q_idx].level,
						quest[q_idx].complev);
				}
			}
			else
			{
				/* Print the quest info */
#ifdef JP
				sprintf(tmp_str, "  %-40s (危険度:%2d階相当) - レベル%2d\n",
					quest[q_idx].name, quest[q_idx].level, quest[q_idx].complev);
#else
				sprintf(tmp_str, "  %-40s (Danger level: %2d)  - level %2d\n",
					quest[q_idx].name, quest[q_idx].level, quest[q_idx].complev);
#endif
			}

			fprintf(fff, tmp_str);
		}
	}
#ifdef JP
	if (!total) fprintf(fff, "  なし\n");
#else
	if (!total) fprintf(fff, "  Nothing.\n");
#endif
}


/*
 * Print all failed quests
 */
void do_cmd_knowledge_quests_failed(FILE *fff, int quest_num[])
{
	char tmp_str[120];
	int i;
	int total = 0;

#ifdef JP
	fprintf(fff, "《失敗したクエスト》\n");
#else
	fprintf(fff, "< Failed Quest >\n");
#endif
	for (i = 1; i < max_quests; i++)
	{
		int q_idx = quest_num[i];

		if ((quest[q_idx].status == QUEST_STATUS_FAILED_DONE) || (quest[q_idx].status == QUEST_STATUS_FAILED))
		{
			if (is_fixed_quest_idx(q_idx))
			{
				/* Set the quest number temporary */
				int old_quest = p_ptr->inside_quest;

				p_ptr->inside_quest = q_idx;

				/* Get the quest text */
				init_flags = INIT_ASSIGN;

				process_dungeon_file("q_info.txt", 0, 0, 0, 0);

				/* Reset the old quest number */
				p_ptr->inside_quest = old_quest;

				/* No info from "silent" quests */
				if (quest[q_idx].flags & QUEST_FLAG_SILENT) continue;
			}

			total++;

			if (!is_fixed_quest_idx(q_idx) && quest[q_idx].r_idx)
			{
				/* Print the quest info */
#ifdef JP
				sprintf(tmp_str, "  %-40s (%2d階)            - レベル%2d\n",
					r_name+r_info[quest[q_idx].r_idx].name, quest[q_idx].level, quest[q_idx].complev);
#else
				sprintf(tmp_str, "  %-40s (Dungeon level: %2d) - level %2d\n",
					r_name+r_info[quest[q_idx].r_idx].name, quest[q_idx].level, quest[q_idx].complev);
#endif
			}
			else
			{
				/* Print the quest info */
#ifdef JP
				sprintf(tmp_str, "  %-40s (危険度:%2d階相当) - レベル%2d\n",
					quest[q_idx].name, quest[q_idx].level, quest[q_idx].complev);
#else
				sprintf(tmp_str, "  %-40s (Danger level: %2d)  - level %2d\n",
					quest[q_idx].name, quest[q_idx].level, quest[q_idx].complev);
#endif
			}
			fprintf(fff, tmp_str);
		}
	}
#ifdef JP
	if (!total) fprintf(fff, "  なし\n");
#else
	if (!total) fprintf(fff, "  Nothing.\n");
#endif
}


/*
 * Print all random quests
 */
static void do_cmd_knowledge_quests_wiz_random(FILE *fff)
{
	char tmp_str[120];
	int i;
	int total = 0;

#ifdef JP
	fprintf(fff, "《残りのランダムクエスト》\n");
#else
	fprintf(fff, "< Remaining Random Quest >\n");
#endif
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
			fprintf(fff, tmp_str);
		}
	}
#ifdef JP
	if (!total) fprintf(fff, "  なし\n");
#else
	if (!total) fprintf(fff, "  Nothing.\n");
#endif
}


bool ang_sort_comp_quest_num(vptr u, vptr v, int a, int b)
{
	int *q_num = (int *)u;
	quest_type *qa = &quest[q_num[a]];
	quest_type *qb = &quest[q_num[b]];

	/* Unused */
	(void)v;

	if (qa->complev < qb->complev) return TRUE;
	if (qa->complev > qb->complev) return FALSE;
	if (qa->level <= qb->level) return TRUE;
	return FALSE;
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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
	show_file(TRUE, file_name, "クエスト達成状況", 0, 0);
#else
	show_file(TRUE, file_name, "Quest status", 0, 0);
#endif

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
#ifdef JP
		msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
		msg_format("Failed to create temporary file %s.", file_name);
#endif
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
#ifdef JP
			fprintf(fff, "  [ 我が家のアイテム ]\n");
#else
			fprintf(fff, "  [Home Inventory]\n");
#endif

			/* Dump all available items */
			for (i = 0; i < st_ptr->stock_num; i++)
			{
#ifdef JP
				if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
				object_desc(o_name, &st_ptr->stock[i], TRUE, 3);
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
				object_desc(o_name, &st_ptr->stock[i], TRUE, 3);
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
#ifdef JP
	show_file(TRUE, file_name, "我が家のアイテム", 0, 0);
#else
	show_file(TRUE, file_name, "Home Inventory", 0, 0);
#endif


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
#ifdef JP
	    msg_format("一時ファイル %s を作成できませんでした。", file_name);
#else
	    msg_format("Failed to create temporary file %s.", file_name);
#endif
	    msg_print(NULL);
	    return;
	}

	if (!max_autopick)
	{
#ifdef JP
	    fprintf(fff, "自動破壊/拾いには何も登録されていません。");
#else
	    fprintf(fff, "No preference for auto picker/destroyer.");
#endif
	}
	else
	{
#ifdef JP
	    fprintf(fff, "   自動拾い/破壊には現在 %d行登録されています。\n\n", max_autopick);
#else
	    fprintf(fff, "   There are %d registered lines for auto picker/destroyer.\n\n", max_autopick);
#endif
	}

	for (k = 0; k < max_autopick; k++)
	{
		cptr tmp;
		byte act = autopick_list[k].action;
		if (act & DONT_AUTOPICK)
		{
#ifdef JP
			tmp = "放置";
#else
			tmp = "Leave";
#endif
		}
		else if (act & DO_AUTODESTROY)
		{
#ifdef JP
			tmp = "破壊";
#else
			tmp = "Destroy";
#endif
		}
		else if (act & DO_AUTOPICK)
		{
#ifdef JP
			tmp = "拾う";
#else
			tmp = "Pickup";
#endif
		}
		else /* if (act & DO_QUERY_AUTOPICK) */ /* Obvious */
		{
#ifdef JP
			tmp = "確認";
#else
			tmp = "Query";
#endif
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
#ifdef JP
	show_file(TRUE, file_name, "自動拾い/破壊 設定リスト", 0, 0);
#else
	show_file(TRUE, file_name, "Auto-picker/Destroyer", 0, 0);
#endif

	/* Remove the file */
	fd_kill(file_name);
}


/*
 * Interact with "knowledge"
 */
void do_cmd_knowledge(void)
{
	int i,p=0;
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
		if (p == 0) {
			prt("(1) 既知の伝説のアイテム                 の一覧", 6, 5);
			prt("(2) 既知のアイテム                       の一覧", 7, 5);
			prt("(3) 既知の生きているユニーク・モンスター の一覧", 8, 5);
			prt("(4) 既知のモンスター                     の一覧", 9, 5);
			prt("(5) 倒した敵の数                         の一覧", 10, 5);
			prt("(6) 賞金首                               の一覧", 11, 5);
			prt("(7) 現在のペット                         の一覧", 12, 5);
			prt("(8) 我が家のアイテム                     の一覧", 13, 5);
			prt("(9) *鑑定*済み装備の耐性                 の一覧", 14, 5);
			prt("(0) 地形の表示文字/タイル                の一覧", 15, 5);
		} else {
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
		if (p == 0) {
			prt("(1) Display known artifacts", 6, 5);
			prt("(2) Display known objects", 7, 5);
			prt("(3) Display remaining uniques", 8, 5);
			prt("(4) Display known monster", 9, 5);
			prt("(5) Display kill count", 10, 5);
			prt("(6) Display wanted monsters", 11, 5);
			prt("(7) Display current pets", 12, 5);
			prt("(8) Display home inventory", 13, 5);
			prt("(9) Display *identified* equip.", 14, 5);
			prt("(0) Display terrain symbols.", 15, 5);
		} else {
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
			do_cmd_knowledge_objects();
			break;
		case '3': /* Uniques */
			do_cmd_knowledge_uniques();
			break;
		case '4': /* Monsters */
			do_cmd_knowledge_monsters();
			break;
		case '5': /* Kill count  */
			do_cmd_knowledge_kill_count();
			break;
		case '6': /* wanted */
			do_cmd_knowledge_kubi();
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
			do_cmd_knowledge_features();
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

	FILE *fff;

	extract_day_hour_min(&day, &hour, &min);

	full = hour * 100 + min;

	start = 9999;
	end = -9999;

	num = 0;

#ifdef JP
	strcpy(desc, "変な時刻だ。");
#else
	strcpy(desc, "It is a strange time.");
#endif


	/* Message */
#ifdef JP
	msg_format("%d 日目,時刻は%d:%02d %sです。",
		   day, (hour % 12 == 0) ? 12 : (hour % 12),
		   min, (hour < 12) ? "AM" : "PM");
#else
	msg_format("This is day %d. The time is %d:%02d %s.",
		   day, (hour % 12 == 0) ? 12 : (hour % 12),
		   min, (hour < 12) ? "AM" : "PM");
#endif


	/* Find the path */
	if (!randint0(10) || p_ptr->image)
	{
#ifdef JP
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "timefun_j.txt");
#else
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "timefun.txt");
#endif

	}
	else
	{
#ifdef JP
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "timenorm_j.txt");
#else
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "timenorm.txt");
#endif

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
