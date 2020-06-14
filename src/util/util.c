/* File: util.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

 /* Purpose: Angband utilities -BEN- */

#include "system/angband.h"
#include "util/util.h"
#include "cmd-io/cmd-dump.h"
#include "cmd-io/cmd-menu-content-table.h"
#include "cmd-io/macro-util.h"
#include "core/asking-player.h"
#include "core/output-updater.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/option-flags.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "locale/japanese.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "player/player-class.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-main-window.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * 10進数から16進数への変換テーブル /
 * Global array for converting numbers to uppercase hecidecimal digit
 * This array can also be used to convert a number to an octal digit
 */
const char hexsym[16] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*
 * Keymaps for each "mode" associated with each keypress.
 */
concptr keymap_act[KEYMAP_MODES][256];

/*
 * Array of macro types [MACRO_MAX]
 */
bool *macro__cmd;

/*
 * Current macro action [1024]
 */
char *macro__buf;

bool get_com_no_macros = FALSE;	/* Expand macros in "get_com" or not */

bool use_menu;

pos_list tmp_pos;

int max_macrotrigger = 0; /*!< 現在登録中のマクロ(トリガー)の数 */
concptr macro_template = NULL; /*!< Angband設定ファイルのT: タグ情報から読み込んだ長いTコードを処理するために利用する文字列ポインタ */
concptr macro_modifier_chr; /*!< &x# で指定されるマクロトリガーに関する情報を記録する文字列ポインタ */
concptr macro_modifier_name[MAX_MACRO_MOD]; /*!< マクロ上で取り扱う特殊キーを文字列上で表現するためのフォーマットを記録した文字列ポインタ配列 */
concptr macro_trigger_name[MAX_MACRO_TRIG]; /*!< マクロのトリガーコード */
concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];  /*!< マクロの内容 */

s16b command_cmd;		/* Current "Angband Command" */
COMMAND_ARG command_arg;	/*!< 各種コマンドの汎用的な引数として扱う / Gives argument of current command */
COMMAND_NUM command_rep;	/*!< 各種コマンドの汎用的なリピート数として扱う / Gives repetition of current command */
DIRECTION command_dir;		/*!< 各種コマンドの汎用的な方向値処理として扱う/ Gives direction of current command */
s16b command_see;		/* アイテム使用時等にリストを表示させるかどうか (ゲームオプションの他、様々なタイミングでONになったりOFFになったりする模様……) */
s16b command_wrk;		/* アイテムの使用許可状況 (ex. 装備品のみ、床上もOK等) */
TERM_LEN command_gap = 999;         /* アイテムの表示に使う (詳細未調査) */
s16b command_new;		/* Command chaining from inven/equip view */

/*
 * Move the cursor
 */
void move_cursor(int row, int col)
{
	Term_gotoxy(col, row);
}

/*
 * Flush all input chars.  Actually, remember the flush,
 * and do a "special flush" before the next "inkey()".
 *
 * This is not only more efficient, but also necessary to make sure
 * that various "inkey()" codes are not "lost" along the way.
 */
void flush(void)
{
	inkey_xtra = TRUE;
}


/*
 * Hack -- prevent "accidents" in "screen_save()" or "screen_load()"
 */
static int screen_depth = 0;


/*
 * Save the screen, and increase the "icky" depth.
 *
 * This function must match exactly one call to "screen_load()".
 */
void screen_save()
{
	msg_print(NULL);
	if (screen_depth++ == 0) Term_save();

	current_world_ptr->character_icky++;
}


/*
 * Load the screen, and decrease the "icky" depth.
 *
 * This function must match exactly one call to "screen_save()".
 */
void screen_load()
{
	msg_print(NULL);
	if (--screen_depth == 0) Term_load();

	current_world_ptr->character_icky--;
}


/*
 * Display a string on the screen using an attribute.
 *
 * At the given location, using the given attribute, if allowed,
 * add the given string.  Do not clear the line.
 */
void c_put_str(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col)
{
	Term_putstr(col, row, -1, attr, str);
}


/*
 * As above, but in "white"
 */
void put_str(concptr str, TERM_LEN row, TERM_LEN col)
{
	Term_putstr(col, row, -1, TERM_WHITE, str);
}


/*
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(TERM_COLOR attr, concptr str, TERM_LEN row, TERM_LEN col)
{
	Term_erase(col, row, 255);
	Term_addstr(-1, attr, str);
}


/*
 * As above, but in "white"
 */
void prt(concptr str, TERM_LEN row, TERM_LEN col)
{
	/* Spawn */
	c_prt(TERM_WHITE, str, row, col);
}


/*
 * Print some (colored) text to the screen at the current cursor position,
 * automatically "wrapping" existing text (at spaces) when necessary to
 * avoid placing any text into the last column, and clearing every line
 * before placing any text in that line.  Also, allow "newline" to force
 * a "wrap" to the next line.  Advance the cursor as needed so sequential
 * calls to this function will work correctly.
 *
 * Once this function has been called, the cursor should not be moved
 * until all the related "c_roff()" calls to the window are complete.
 *
 * This function will correctly handle any width up to the maximum legal
 * value of 256, though it works best for a standard 80 character width.
 */
void c_roff(TERM_COLOR a, concptr str)
{
	int w, h;
	(void)Term_get_size(&w, &h);

	int x, y;
	(void)Term_locate(&x, &y);

	if (y == h - 1 && x > w - 3) return;

	for (concptr s = str; *s; s++)
	{
		char ch;
#ifdef JP
		int k_flag = iskanji(*s);
#endif
		if (*s == '\n')
		{
			x = 0;
			y++;
			if (y == h) break;

			Term_erase(x, y, 255);
			break;
		}

#ifdef JP
		ch = ((k_flag || isprint(*s)) ? *s : ' ');
#else
		ch = (isprint(*s) ? *s : ' ');
#endif

#ifdef JP
		if ((x >= ((k_flag) ? w - 2 : w - 1)) && (ch != ' '))
#else
		if ((x >= w - 1) && (ch != ' '))
#endif
		{
			int i, n = 0;

			TERM_COLOR av[256];
			char cv[256];
			if (x < w)
#ifdef JP
			{
				/* 現在が半角文字の場合 */
				if (!k_flag)
#endif
				{
					for (i = w - 2; i >= 0; i--)
					{
						Term_what(i, y, &av[i], &cv[i]);
						if (cv[i] == ' ') break;

						n = i;
#ifdef JP
						if (cv[i] == '(') break;
#endif
					}
				}
#ifdef JP
				else
				{
					/* 現在が全角文字のとき */
					/* 文頭が「。」「、」等になるときは、その１つ前の語で改行 */
					if (strncmp(s, "。", 2) == 0 || strncmp(s, "、", 2) == 0)
					{
						Term_what(x, y, &av[x], &cv[x]);
						Term_what(x - 1, y, &av[x - 1], &cv[x - 1]);
						Term_what(x - 2, y, &av[x - 2], &cv[x - 2]);
						n = x - 2;
						cv[x] = '\0';
					}
				}
			}
#endif
			if (n == 0) n = w;

			Term_erase(n, y, 255);
			x = 0;
			y++;
			if (y == h) break;

			Term_erase(x, y, 255);
			for (i = n; i < w - 1; i++)
			{
#ifdef JP
				if (cv[i] == '\0') break;
#endif
				Term_addch(av[i], cv[i]);
				if (++x > w) x = w;
			}
		}

#ifdef JP
		Term_addch((byte)(a | 0x10), ch);
#else
		Term_addch(a, ch);
#endif

#ifdef JP
		if (k_flag)
		{
			s++;
			x++;
			ch = *s;
			Term_addch((byte)(a | 0x20), ch);
		}
#endif

		if (++x > w) x = w;
	}
}


/*
 * As above, but in "white"
 */
void roff(concptr str)
{
	/* Spawn */
	c_roff(TERM_WHITE, str);
}


/*
 * Clear part of the screen
 */
void clear_from(int row)
{
	for (int y = row; y < Term->hgt; y++)
	{
		Term_erase(0, y, 255);
	}
}


/*
 * Hack -- special buffer to hold the action of the current keymap
 */
static char request_command_buffer[256];

static char inkey_from_menu(player_type *player_ptr)
{
	char cmd;
	int basey, basex;
	int num = 0, max_num, old_num = 0;
	int menu = 0;
	bool kisuu;

	if (player_ptr->y - panel_row_min > 10) basey = 2;
	else basey = 13;
	basex = 15;

	prt("", 0, 0);
	screen_save();

	floor_type* floor_ptr = player_ptr->current_floor_ptr;
	while (TRUE)
	{
		int i;
		char sub_cmd;
		concptr menu_name;
		if (!menu) old_num = num;
		put_str("+----------------------------------------------------+", basey, basex);
		put_str("|                                                    |", basey + 1, basex);
		put_str("|                                                    |", basey + 2, basex);
		put_str("|                                                    |", basey + 3, basex);
		put_str("|                                                    |", basey + 4, basex);
		put_str("|                                                    |", basey + 5, basex);
		put_str("+----------------------------------------------------+", basey + 6, basex);

		for (i = 0; i < 10; i++)
		{
			int hoge;
			if (!menu_info[menu][i].cmd) break;
			menu_name = menu_info[menu][i].name;
			for (hoge = 0; ; hoge++)
			{
				if (!special_menu_info[hoge].name[0]) break;
				if ((menu != special_menu_info[hoge].window) || (i != special_menu_info[hoge].number)) continue;
				switch (special_menu_info[hoge].jouken)
				{
				case MENU_CLASS:
					if (player_ptr->pclass == special_menu_info[hoge].jouken_naiyou) menu_name = special_menu_info[hoge].name;
					break;
				case MENU_WILD:
					if (!floor_ptr->dun_level && !floor_ptr->inside_arena && !floor_ptr->inside_quest)
					{
						if ((byte)player_ptr->wild_mode == special_menu_info[hoge].jouken_naiyou) menu_name = special_menu_info[hoge].name;
					}
					break;
				default:
					break;
				}
			}

			put_str(menu_name, basey + 1 + i / 2, basex + 4 + (i % 2) * 24);
		}

		max_num = i;
		kisuu = max_num % 2;
		put_str(_("》", "> "), basey + 1 + num / 2, basex + 2 + (num % 2) * 24);

		move_cursor_relative(player_ptr->y, player_ptr->x);
		sub_cmd = inkey();
		if ((sub_cmd == ' ') || (sub_cmd == 'x') || (sub_cmd == 'X') || (sub_cmd == '\r') || (sub_cmd == '\n'))
		{
			if (menu_info[menu][num].fin)
			{
				cmd = menu_info[menu][num].cmd;
				use_menu = TRUE;
				break;
			}
			else
			{
				menu = menu_info[menu][num].cmd;
				num = 0;
				basey += 2;
				basex += 8;
			}
		}
		else if ((sub_cmd == ESCAPE) || (sub_cmd == 'z') || (sub_cmd == 'Z') || (sub_cmd == '0'))
		{
			if (!menu)
			{
				cmd = ESCAPE;
				break;
			}
			else
			{
				menu = 0;
				num = old_num;
				basey -= 2;
				basex -= 8;
				screen_load();
				screen_save();
			}
		}
		else if ((sub_cmd == '2') || (sub_cmd == 'j') || (sub_cmd == 'J'))
		{
			if (kisuu)
			{
				if (num % 2)
					num = (num + 2) % (max_num - 1);
				else
					num = (num + 2) % (max_num + 1);
			}
			else num = (num + 2) % max_num;
		}
		else if ((sub_cmd == '8') || (sub_cmd == 'k') || (sub_cmd == 'K'))
		{
			if (kisuu)
			{
				if (num % 2)
					num = (num + max_num - 3) % (max_num - 1);
				else
					num = (num + max_num - 1) % (max_num + 1);
			}
			else num = (num + max_num - 2) % max_num;
		}
		else if ((sub_cmd == '4') || (sub_cmd == '6') || (sub_cmd == 'h') || (sub_cmd == 'H') || (sub_cmd == 'l') || (sub_cmd == 'L'))
		{
			if ((num % 2) || (num == max_num - 1))
			{
				num--;
			}
			else if (num < max_num - 1)
			{
				num++;
			}
		}
	}

	screen_load();
	if (!inkey_next) inkey_next = "";

	return (cmd);
}


/*
 * Request a command from the user.
 *
 * Sets player_ptr->command_cmd, player_ptr->command_dir, player_ptr->command_rep,
 * player_ptr->command_arg.  May modify player_ptr->command_new.
 *
 * Note that "caret" ("^") is treated specially, and is used to
 * allow manual input of control characters.  This can be used
 * on many machines to request repeated tunneling (Ctrl-H) and
 * on the Macintosh to request "Control-Caret".
 *
 * Note that "backslash" is treated specially, and is used to bypass any
 * keymap entry for the following character.  This is useful for macros.
 *
 * Note that this command is used both in the dungeon and in
 * stores, and must be careful to work in both situations.
 *
 * Note that "player_ptr->command_new" may not work any more.
 */
void request_command(player_type *player_ptr, int shopping)
{
	s16b cmd;
	int mode;

	concptr act;

#ifdef JP
	int caretcmd = 0;
#endif
	if (rogue_like_commands)
	{
		mode = KEYMAP_MODE_ROGUE;
	}
	else
	{
		mode = KEYMAP_MODE_ORIG;
	}

	command_cmd = 0;
	command_arg = 0;
	command_dir = 0;
	use_menu = FALSE;

	while (TRUE)
	{
		if (command_new)
		{
			msg_erase();
			cmd = command_new;
			command_new = 0;
		}
		else
		{
			msg_flag = FALSE;
			num_more = 0;
			inkey_flag = TRUE;
			cmd = inkey();
			if (!shopping && command_menu && ((cmd == '\r') || (cmd == '\n') || (cmd == 'x') || (cmd == 'X'))
				&& !keymap_act[mode][(byte)(cmd)])
				cmd = inkey_from_menu(player_ptr);
		}

		prt("", 0, 0);
		if (cmd == '0')
		{
			COMMAND_ARG old_arg = command_arg;
			command_arg = 0;
			prt(_("回数: ", "Count: "), 0, 0);
			while (TRUE)
			{
				cmd = inkey();
				if ((cmd == 0x7F) || (cmd == KTRL('H')))
				{
					command_arg = command_arg / 10;
					prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
				}
				else if (cmd >= '0' && cmd <= '9')
				{
					if (command_arg >= 1000)
					{
						bell();
						command_arg = 9999;
					}
					else
					{
						command_arg = command_arg * 10 + D2I(cmd);
					}

					prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
				}
				else
				{
					break;
				}
			}

			if (command_arg == 0)
			{
				command_arg = 99;
				prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
			}

			if (old_arg != 0)
			{
				command_arg = old_arg;
				prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
			}

			if ((cmd == ' ') || (cmd == '\n') || (cmd == '\r'))
			{
				if (!get_com(_("コマンド: ", "Command: "), (char *)&cmd, FALSE))
				{
					command_arg = 0;
					continue;
				}
			}
		}

		if (cmd == '\\')
		{
			(void)get_com(_("コマンド: ", "Command: "), (char *)&cmd, FALSE);
			if (!inkey_next) inkey_next = "";
		}

		if (cmd == '^')
		{
			if (get_com(_("CTRL: ", "Control: "), (char *)&cmd, FALSE)) cmd = KTRL(cmd);
		}

		act = keymap_act[mode][(byte)(cmd)];
		if (act && !inkey_next)
		{
			(void)strnfmt(request_command_buffer, 256, "%s", act);
			inkey_next = request_command_buffer;
			continue;
		}

		if (!cmd) continue;

		command_cmd = (byte)cmd;
		break;
	}

	if (always_repeat && (command_arg <= 0))
	{
		if (angband_strchr("TBDoc+", (char)command_cmd))
		{
			command_arg = 99;
		}
	}

	if (shopping == 1)
	{
		switch (command_cmd)
		{
		case 'p': command_cmd = 'g'; break;

		case 'm': command_cmd = 'g'; break;

		case 's': command_cmd = 'd'; break;
		}
	}

#ifdef JP
	for (int i = 0; i < 256; i++)
	{
		concptr s;
		if ((s = keymap_act[mode][i]) != NULL)
		{
			if (*s == command_cmd && *(s + 1) == 0)
			{
				caretcmd = i;
				break;
			}
		}
	}

	if (!caretcmd)
		caretcmd = command_cmd;
#endif

	for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &player_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		if (!o_ptr->inscription) continue;

		concptr s = quark_str(o_ptr->inscription);
		s = angband_strchr(s, '^');
		while (s)
		{
#ifdef JP
			if ((s[1] == caretcmd) || (s[1] == '*'))
#else
			if ((s[1] == command_cmd) || (s[1] == '*'))
#endif
			{
				if (!get_check(_("本当ですか? ", "Are you sure? ")))
				{
					command_cmd = ' ';
				}
			}

			s = angband_strchr(s + 1, '^');
		}
	}

	prt("", 0, 0);
}


/*
 * Check a char for "vowel-hood"
 */
bool is_a_vowel(int ch)
{
	switch (ch)
	{
	case 'a':
	case 'e':
	case 'i':
	case 'o':
	case 'u':
	case 'A':
	case 'E':
	case 'I':
	case 'O':
	case 'U':
		return TRUE;
	}

	return FALSE;
}


/*
 * GH
 * Called from cmd4.c and a few other places. Just extracts
 * a direction from the keymap for ch (the last direction,
 * in fact) byte or char here? I'm thinking that keymaps should
 * generally only apply to single keys, which makes it no more
 * than 128, so a char should suffice... but keymap_act is 256...
 */
int get_keymap_dir(char ch)
{
	int d = 0;

	if (isdigit(ch))
	{
		d = D2I(ch);
	}
	else
	{
		BIT_FLAGS mode;
		if (rogue_like_commands)
		{
			mode = KEYMAP_MODE_ROGUE;
		}
		else
		{
			mode = KEYMAP_MODE_ORIG;
		}

		concptr act = keymap_act[mode][(byte)(ch)];
		if (act)
		{
			for (concptr s = act; *s; ++s)
			{
				if (isdigit(*s)) d = D2I(*s);
			}
		}
	}

	if (d == 5) d = 0;

	return (d);
}


#define REPEAT_MAX		20

/* Number of chars saved */
static int repeat__cnt = 0;

/* Current index */
static int repeat__idx = 0;

/* Saved "stuff" */
static COMMAND_CODE repeat__key[REPEAT_MAX];

void repeat_push(COMMAND_CODE what)
{
	if (repeat__cnt == REPEAT_MAX) return;

	repeat__key[repeat__cnt++] = what;
	++repeat__idx;
}


bool repeat_pull(COMMAND_CODE *what)
{
	if (repeat__idx == repeat__cnt) return FALSE;

	*what = repeat__key[repeat__idx++];
	return TRUE;
}

void repeat_check(void)
{
	if (command_cmd == ESCAPE) return;
	if (command_cmd == ' ') return;
	if (command_cmd == '\r') return;
	if (command_cmd == '\n') return;

	COMMAND_CODE what;
	if (command_cmd == 'n')
	{
		repeat__idx = 0;
		if (repeat_pull(&what))
		{
			command_cmd = what;
		}
	}
	else
	{
		repeat__cnt = 0;
		repeat__idx = 0;
		what = command_cmd;
		repeat_push(what);
	}
}


/*
 * Array size for which InsertionSort
 * is used instead of QuickSort
 */
#define CUTOFF 4


 /*
  * Exchange two sort-entries
  * (should probably be coded inline
  * for speed increase)
  */
static void swap(tag_type *a, tag_type *b)
{
	tag_type temp;

	temp = *a;
	*a = *b;
	*b = temp;
}


/*
 * Insertion-Sort algorithm
 * (used by the Quicksort algorithm)
 */
static void InsertionSort(tag_type elements[], int number)
{
	tag_type tmp;
	for (int i = 1; i < number; i++)
	{
		tmp = elements[i];
		int j;
		for (j = i; (j > 0) && (elements[j - 1].tag > tmp.tag); j--)
			elements[j] = elements[j - 1];
		elements[j] = tmp;
	}
}


/*
 * Helper function for Quicksort
 */
static tag_type median3(tag_type elements[], int left, int right)
{
	int center = (left + right) / 2;

	if (elements[left].tag > elements[center].tag)
		swap(&elements[left], &elements[center]);
	if (elements[left].tag > elements[right].tag)
		swap(&elements[left], &elements[right]);
	if (elements[center].tag > elements[right].tag)
		swap(&elements[center], &elements[right]);

	swap(&elements[center], &elements[right - 1]);
	return (elements[right - 1]);
}


/*
 * Quicksort algorithm
 *
 * The "median of three" pivot selection eliminates
 * the bad case of already sorted input.
 *
 * We use InsertionSort for smaller sub-arrays,
 * because it is faster in this case.
 *
 * For details see: "Data Structures and Algorithm
 * Analysis in C" by Mark Allen Weiss.
 */
static void quicksort(tag_type elements[], int left, int right)
{
	tag_type pivot;
	if (left + CUTOFF <= right)
	{
		pivot = median3(elements, left, right);

		int i = left;
		int j = right - 1;

		while (TRUE)
		{
			while (elements[++i].tag < pivot.tag);
			while (elements[--j].tag > pivot.tag);

			if (i < j)
				swap(&elements[i], &elements[j]);
			else
				break;
		}

		swap(&elements[i], &elements[right - 1]);

		quicksort(elements, left, i - 1);
		quicksort(elements, i + 1, right);
	}
	else
	{
		InsertionSort(elements + left, right - left + 1);
	}
}


/*
 * Frontend for the sorting algorithm
 *
 * Sorts an array of tagged pointers
 * with <number> elements.
 */
void tag_sort(tag_type elements[], int number)
{
	quicksort(elements, 0, number - 1);
}

/*
 * Add a series of keypresses to the "queue".
 *
 * Return any errors generated by Term_keypress() in doing so, or SUCCESS
 * if there are none.
 *
 * Catch the "out of space" error before anything is printed.
 *
 * NB: The keys added here will be interpreted by any macros or keymaps.
 */
errr type_string(concptr str, uint len)
{
	errr err = 0;
	term *old = Term;
	if (!str) return -1;
	if (!len) len = strlen(str);

	Term_activate(term_screen);
	for (concptr s = str; s < str + len; s++)
	{
		if (*s == '\0') break;

		err = Term_keypress(*s);
		if (err) break;
	}

	Term_activate(old);
	return err;
}


void roff_to_buf(concptr str, int maxlen, char *tbuf, size_t bufsize)
{
	int read_pt = 0;
	int write_pt = 0;
	int line_len = 0;
	int word_punct = 0;
	char ch[3];
	ch[2] = '\0';

	while (str[read_pt])
	{
#ifdef JP
		bool kinsoku = FALSE;
		bool kanji;
#endif
		int ch_len = 1;
		ch[0] = str[read_pt];
		ch[1] = '\0';
#ifdef JP
		kanji = iskanji(ch[0]);

		if (kanji)
		{
			ch[1] = str[read_pt + 1];
			ch_len = 2;

			if (strcmp(ch, "。") == 0 ||
				strcmp(ch, "、") == 0 ||
				strcmp(ch, "ィ") == 0 ||
				strcmp(ch, "ー") == 0)
				kinsoku = TRUE;
		}
		else if (!isprint(ch[0]))
			ch[0] = ' ';
#else
		if (!isprint(ch[0]))
			ch[0] = ' ';
#endif

		if (line_len + ch_len > maxlen - 1 || str[read_pt] == '\n')
		{
			int word_len = read_pt - word_punct;
#ifdef JP
			if (kanji && !kinsoku)
				/* nothing */;
			else
#endif
				if (ch[0] == ' ' || word_len >= line_len / 2)
					read_pt++;
				else
				{
					read_pt = word_punct;
					if (str[word_punct] == ' ')
						read_pt++;
					write_pt -= word_len;
				}

			tbuf[write_pt++] = '\0';
			line_len = 0;
			word_punct = read_pt;
			continue;
		}

		if (ch[0] == ' ')
			word_punct = read_pt;

#ifdef JP
		if (!kinsoku) word_punct = read_pt;
#endif

		if ((size_t)(write_pt + 3) >= bufsize) break;

		tbuf[write_pt++] = ch[0];
		line_len++;
		read_pt++;
#ifdef JP
		if (kanji)
		{
			tbuf[write_pt++] = ch[1];
			line_len++;
			read_pt++;
		}
#endif
	}

	tbuf[write_pt] = '\0';
	tbuf[write_pt + 1] = '\0';
	return;
}


/*
 * The angband_strcpy() function copies up to 'bufsize'-1 characters from 'src'
 * to 'buf' and NUL-terminates the result.  The 'buf' and 'src' strings may
 * not overlap.
 *
 * angband_strcpy() returns strlen(src).  This makes checking for truncation
 * easy.  Example: if (angband_strcpy(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcpy() function in BSD.
 */
size_t angband_strcpy(char *buf, concptr src, size_t bufsize)
{
#ifdef JP
	char *d = buf;
	concptr s = src;
	size_t len = 0;

	if (bufsize > 0) {
		/* reserve for NUL termination */
		bufsize--;

		/* Copy as many bytes as will fit */
		while (*s && (len < bufsize))
		{
			if (iskanji(*s))
			{
				if (len + 1 >= bufsize || !*(s + 1)) break;
				*d++ = *s++;
				*d++ = *s++;
				len += 2;
			}
			else
			{
				*d++ = *s++;
				len++;
			}
		}
		*d = '\0';
	}

	while (*s++) len++;
	return len;

#else
	size_t len = strlen(src);
	size_t ret = len;
	if (bufsize == 0) return ret;

	if (len >= bufsize) len = bufsize - 1;

	(void)memcpy(buf, src, len);
	buf[len] = '\0';
	return ret;
#endif
}


/*
 * The angband_strcat() tries to append a string to an existing NUL-terminated string.
 * It never writes more characters into the buffer than indicated by 'bufsize' and
 * NUL-terminates the buffer.  The 'buf' and 'src' strings may not overlap.
 *
 * angband_strcat() returns strlen(buf) + strlen(src).  This makes checking for
 * truncation easy.  Example:
 * if (angband_strcat(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcat() function in BSD.
 */
size_t angband_strcat(char *buf, concptr src, size_t bufsize)
{
	size_t dlen = strlen(buf);
	if (dlen < bufsize - 1)
	{
		return (dlen + angband_strcpy(buf + dlen, src, bufsize - dlen));
	}
	else
	{
		return (dlen + strlen(src));
	}
}


/*
 * A copy of ANSI strstr()
 *
 * angband_strstr() can handle Kanji strings correctly.
 */
char *angband_strstr(concptr haystack, concptr needle)
{
	int l1 = strlen(haystack);
	int l2 = strlen(needle);

	if (l1 >= l2)
	{
		for (int i = 0; i <= l1 - l2; i++)
		{
			if (!strncmp(haystack + i, needle, l2))
				return (char *)haystack + i;

#ifdef JP
			if (iskanji(*(haystack + i))) i++;
#endif
		}
	}

	return NULL;
}


/*
 * A copy of ANSI strchr()
 *
 * angband_strchr() can handle Kanji strings correctly.
 */
char *angband_strchr(concptr ptr, char ch)
{
	for (; *ptr != '\0'; ptr++)
	{
		if (*ptr == ch) return (char *)ptr;

#ifdef JP
		if (iskanji(*ptr)) ptr++;
#endif
	}

	return NULL;
}


/*
 * Convert string to lower case
 */
void str_tolower(char *str)
{
	for (; *str; str++)
	{
#ifdef JP
		if (iskanji(*str))
		{
			str++;
			continue;
		}
#endif
		*str = (char)tolower(*str);
	}
}
