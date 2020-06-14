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
#include "io/input-key-processor.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "locale/japanese.h"
#include "main/music-definitions-table.h"
#include "monster-race/monster-race-hook.h"
#include "player/player-class.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include "view/display-main-window.h"
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
 * The next "free" index to use
 */
u32b message__next;

/*
 * The index of the oldest message (none yet)
 */
u32b message__last;

/*
 * The next "free" offset
 */
u32b message__head;

/*
 * The offset to the oldest used char (none yet)
 */
u32b message__tail;

/*
 * The array of offsets, by index [MESSAGE_MAX]
 */
u32b *message__ptr;

/*
 * The array of chars, by offset [MESSAGE_BUF]
 */
char *message__buf;

bool msg_flag;			/* Used in msg_print() for "buffering" */

/*
 * Array of macro types [MACRO_MAX]
 */
bool *macro__cmd;

/*
 * Current macro action [1024]
 */
char *macro__buf;

bool get_com_no_macros = FALSE;	/* Expand macros in "get_com" or not */

bool inkey_base;		/* See the "inkey()" function */
bool inkey_xtra;		/* See the "inkey()" function */
bool inkey_scan;		/* See the "inkey()" function */
bool inkey_flag;		/* See the "inkey()" function */

bool use_menu;

pos_list tmp_pos;

/*
 * The number of quarks
 */
STR_OFFSET quark__num;

/*
 * The pointers to the quarks [QUARK_MAX]
 */
concptr *quark__str;

static int num_more = 0;

/* Save macro trigger string for use in inkey_special() */
static char inkey_macro_trigger_string[1024];

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
 * Local variable -- we are inside a "macro action"
 *
 * Do not match any macros until "ascii 30" is found.
 */
static bool parse_macro = FALSE;

/*
 * Local variable -- we are inside a "macro trigger"
 *
 * Strip all keypresses until a low ascii value is found.
 */
static bool parse_under = FALSE;

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
 * Flush the screen, make a noise
 */
void bell(void)
{
	Term_fresh();
	if (ring_bell) Term_xtra(TERM_XTRA_NOISE, 0);

	flush();
}


/*
 * todo intをsound_typeに差し替える
 * @brief 音を鳴らす
 */
void sound(int val)
{
	if (!use_sound) return;

	Term_xtra(TERM_XTRA_SOUND, val);
}


/*
 * Hack -- Play a music
 */
errr play_music(int type, int val)
{
	if (!use_music) return 1;

	return Term_xtra(type, val);
}


/*
 * Hack -- Select floor music.
 */
void select_floor_music(player_type *player_ptr)
{
	if (!use_music) return;

	if (player_ptr->ambush_flag)
	{
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_AMBUSH)) return;
	}

	if (player_ptr->wild_mode)
	{
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_WILD)) return;
	}

	if (player_ptr->current_floor_ptr->inside_arena)
	{
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_ARENA)) return;
	}

	if (player_ptr->phase_out)
	{
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_BATTLE)) return;
	}

	if (player_ptr->current_floor_ptr->inside_quest)
	{
		if (!play_music(TERM_XTRA_MUSIC_QUEST, player_ptr->current_floor_ptr->inside_quest)) return;
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST)) return;
	}

	if (player_ptr->dungeon_idx)
	{
		if (player_ptr->feeling == 2)
		{
			if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_FEEL2)) return;
		}
		else if (player_ptr->feeling >= 3 && player_ptr->feeling <= 5)
		{
			if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_FEEL1)) return;
		}
		else
		{
			if (!play_music(TERM_XTRA_MUSIC_DUNGEON, player_ptr->dungeon_idx)) return;

			if (player_ptr->current_floor_ptr->dun_level < 40)
			{
				if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_LOW)) return;
			}
			else if (player_ptr->current_floor_ptr->dun_level < 80)
			{
				if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_MED)) return;
			}
			else
			{
				if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DUN_HIGH)) return;
			}
		}
	}

	if (player_ptr->town_num)
	{
		if (!play_music(TERM_XTRA_MUSIC_TOWN, player_ptr->town_num)) return;
		if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_TOWN)) return;
		return;
	}

	if (!player_ptr->current_floor_ptr->dun_level)
	{
		if (player_ptr->lev >= 45)
		{
			if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD3)) return;
		}
		else if (player_ptr->lev >= 25)
		{
			if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD2)) return;
		}
		else
		{
			if (!play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_FIELD1)) return;
		}
	}

	play_music(TERM_XTRA_MUSIC_MUTE, 0);
}



/*
 * Helper function called only from "inkey()"
 *
 * This function does almost all of the "macro" processing.
 *
 * We use the "Term_key_push()" function to handle "failed" macros, as well
 * as "extra" keys read in while choosing the proper macro, and also to hold
 * the action for the macro, plus a special "ascii 30" character indicating
 * that any macro action in progress is complete.  Embedded macros are thus
 * illegal, unless a macro action includes an explicit "ascii 30" character,
 * which would probably be a massive hack, and might break things.
 *
 * Only 500 (0+1+2+...+29+30) milliseconds may elapse between each key in
 * the macro trigger sequence.  If a key sequence forms the "prefix" of a
 * macro trigger, 500 milliseconds must pass before the key sequence is
 * known not to be that macro trigger.
 */
static char inkey_aux(void)
{
	int k = 0, n, p = 0, w = 0;
	char ch;
	char *buf = inkey_macro_trigger_string;

	num_more = 0;

	if (parse_macro)
	{
		if (Term_inkey(&ch, FALSE, TRUE))
		{
			parse_macro = FALSE;
		}
	}
	else
	{
		(void)(Term_inkey(&ch, TRUE, TRUE));
	}

	if (ch == 30) parse_macro = FALSE;

	if (ch == 30) return (ch);
	if (parse_macro) return (ch);
	if (parse_under) return (ch);

	buf[p++] = ch;
	buf[p] = '\0';
	k = macro_find_check(buf);
	if (k < 0) return (ch);

	while (TRUE)
	{
		k = macro_find_maybe(buf);

		if (k < 0) break;

		if (0 == Term_inkey(&ch, FALSE, TRUE))
		{
			buf[p++] = ch;
			buf[p] = '\0';
			w = 0;
		}
		else
		{
			w += 1;
			if (w >= 10) break;

			Term_xtra(TERM_XTRA_DELAY, w);
		}
	}

	k = macro_find_ready(buf);
	if (k < 0)
	{
		while (p > 0)
		{
			if (Term_key_push(buf[--p])) return 0;
		}

		(void)Term_inkey(&ch, TRUE, TRUE);
		return (ch);
	}

	concptr pat = macro__pat[k];
	n = strlen(pat);
	while (p > n)
	{
		if (Term_key_push(buf[--p])) return 0;
	}

	parse_macro = TRUE;
	if (Term_key_push(30)) return 0;

	concptr act = macro__act[k];

	n = strlen(act);
	while (n > 0)
	{
		if (Term_key_push(act[--n])) return 0;
	}

	return 0;
}


/*
 * Cancel macro action on the queue
 */
static void forget_macro_action(void)
{
	if (!parse_macro) return;

	while (TRUE)
	{
		char ch;
		if (Term_inkey(&ch, FALSE, TRUE)) break;
		if (ch == 0) break;
		if (ch == 30) break;
	}

	parse_macro = FALSE;
}

/*
 * Mega-Hack -- special "inkey_next" pointer.
 *
 * This special pointer allows a sequence of keys to be "inserted" into
 * the stream of keys returned by "inkey()".  This key sequence will not
 * trigger any macros, and cannot be bypassed by the Borg.  It is used
 * in Angband to handle "keymaps".
 */
static concptr inkey_next = NULL;

/*
 * Get a keypress from the user.
 *
 * This function recognizes a few "global parameters".  These are variables
 * which, if set to TRUE before calling this function, will have an effect
 * on this function, and which are always reset to FALSE by this function
 * before this function returns.  Thus they function just like normal
 * parameters, except that most calls to this function can ignore them.
 *
 * If "inkey_xtra" is TRUE, then all pending keypresses will be flushed,
 * and any macro processing in progress will be aborted.  This flag is
 * set by the "flush()" function, which does not actually flush anything
 * itself, but rather, triggers delayed input flushing via "inkey_xtra".
 *
 * If "inkey_scan" is TRUE, then we will immediately return "zero" if no
 * keypress is available, instead of waiting for a keypress.
 *
 * If "inkey_base" is TRUE, then all macro processing will be bypassed.
 * If "inkey_base" and "inkey_scan" are both TRUE, then this function will
 * not return immediately, but will wait for a keypress for as long as the
 * normal macro matching code would, allowing the direct entry of macro
 * triggers.  The "inkey_base" flag is extremely dangerous!
 *
 * If "inkey_flag" is TRUE, then we will assume that we are waiting for a
 * normal command, and we will only show the cursor if "hilite_player" is
 * TRUE (or if the player is in a store), instead of always showing the
 * cursor.  The various "main-xxx.c" files should avoid saving the game
 * in response to a "menu item" request unless "inkey_flag" is TRUE, to
 * prevent savefile corruption.
 *
 * If we are waiting for a keypress, and no keypress is ready, then we will
 * refresh (once) the window which was active when this function was called.
 *
 * Note that "back-quote" is automatically converted into "escape" for
 * convenience on machines with no "escape" key.  This is done after the
 * macro matching, so the user can still make a macro for "backquote".
 *
 * Note the special handling of "ascii 30" (ctrl-caret, aka ctrl-shift-six)
 * and "ascii 31" (ctrl-underscore, aka ctrl-shift-minus), which are used to
 * provide support for simple keyboard "macros".  These keys are so strange
 * that their loss as normal keys will probably be noticed by nobody.  The
 * "ascii 30" key is used to indicate the "end" of a macro action, which
 * allows recursive macros to be avoided.  The "ascii 31" key is used by
 * some of the "main-xxx.c" files to introduce macro trigger sequences.
 *
 * Hack -- we use "ascii 29" (ctrl-right-bracket) as a special "magic" key,
 * which can be used to give a variety of "sub-commands" which can be used
 * any time.  These sub-commands could include commands to take a picture of
 * the current screen, to start/stop recording a macro action, etc.
 *
 * If "angband_term[0]" is not active, we will make it active during this
 * function, so that the various "main-xxx.c" files can assume that input
 * is only requested (via "Term_inkey()") when "angband_term[0]" is active.
 *
 * Mega-Hack -- This function is used as the entry point for clearing the
 * "signal_count" variable, and of the "current_world_ptr->character_saved" variable.
 *
 * Hack -- Note the use of "inkey_next" to allow "keymaps" to be processed.
 *
 * Mega-Hack -- Note the use of "inkey_hack" to allow the "Borg" to steal
 * control of the keyboard from the user.
 */
char inkey(void)
{
	char ch = 0;
	bool done = FALSE;
	term *old = Term;

	if (inkey_next && *inkey_next && !inkey_xtra)
	{
		ch = *inkey_next++;
		inkey_base = inkey_xtra = inkey_flag = inkey_scan = FALSE;
		return (ch);
	}

	inkey_next = NULL;
	if (inkey_xtra)
	{
		parse_macro = FALSE;
		parse_under = FALSE;
		Term_flush();
	}

	int v;
	(void)Term_get_cursor(&v);

	/* Show the cursor if waiting, except sometimes in "command" mode */
	if (!inkey_scan && (!inkey_flag || hilite_player || current_world_ptr->character_icky))
	{
		(void)Term_set_cursor(1);
	}

	Term_activate(angband_term[0]);
	char kk;
	while (!ch)
	{
		if (!inkey_base && inkey_scan &&
			(0 != Term_inkey(&kk, FALSE, FALSE)))
		{
			break;
		}

		if (!done && (0 != Term_inkey(&kk, FALSE, FALSE)))
		{
			Term_activate(old);
			Term_fresh();
			Term_activate(angband_term[0]);
			current_world_ptr->character_saved = FALSE;

			signal_count = 0;
			done = TRUE;
		}

		if (inkey_base)
		{
			int w = 0;
			if (!inkey_scan)
			{
				if (0 == Term_inkey(&ch, TRUE, TRUE))
				{
					break;
				}

				break;
			}

			while (TRUE)
			{
				if (0 == Term_inkey(&ch, FALSE, TRUE))
				{
					break;
				}
				else
				{
					w += 10;
					if (w >= 100) break;

					Term_xtra(TERM_XTRA_DELAY, w);
				}
			}

			break;
		}

		ch = inkey_aux();
		if (ch == 29)
		{
			ch = 0;
			continue;
		}

		if (parse_under && (ch <= 32))
		{
			ch = 0;
			parse_under = FALSE;
		}

		if (ch == 30)
		{
			ch = 0;
		}
		else if (ch == 31)
		{
			ch = 0;
			parse_under = TRUE;
		}
		else if (parse_under)
		{
			ch = 0;
		}
	}

	Term_activate(old);
	Term_set_cursor(v);
	inkey_base = inkey_xtra = inkey_flag = inkey_scan = FALSE;
	return (ch);
}


 /*
  * Initialize the quark array
  */
void quark_init(void)
{
	C_MAKE(quark__str, QUARK_MAX, concptr);
	quark__str[1] = string_make("");
	quark__num = 2;
}


/*
 * Add a new "quark" to the set of quarks.
 */
u16b quark_add(concptr str)
{
	u16b i;
	for (i = 1; i < quark__num; i++)
	{
		if (streq(quark__str[i], str)) return (i);
	}

	if (quark__num == QUARK_MAX) return 1;

	quark__num = i + 1;
	quark__str[i] = string_make(str);
	return (i);
}


/*
 * This function looks up a quark
 */
concptr quark_str(STR_OFFSET i)
{
	concptr q;

	/* Return NULL for an invalid index */
	if ((i < 1) || (i >= quark__num)) return NULL;

	/* Access the quark */
	q = quark__str[i];

	/* Return the quark */
	return (q);
}


 /*!
  * @brief 保存中の過去ゲームメッセージの数を返す。 / How many messages are "available"?
  * @return 残っているメッセージの数
  */
s32b message_num(void)
{
	int n;
	int last = message__last;
	int next = message__next;

	if (next < last) next += MESSAGE_MAX;

	n = (next - last);
	return (n);
}


/*!
 * @brief 過去のゲームメッセージを返す。 / Recall the "text" of a saved message
 * @params age メッセージの世代
 * @return メッセージの文字列ポインタ
 */
concptr message_str(int age)
{
	if ((age < 0) || (age >= message_num())) return ("");

	s32b x = (message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX;
	s32b o = message__ptr[x];
	concptr s = &message__buf[o];
	return (s);
}


/*!
 * @brief ゲームメッセージをログに追加する。 / Add a new message, with great efficiency
 * @params str 保存したいメッセージ
 * @return なし
 */
void message_add(concptr str)
{
	u32b i;
	int x, m;
	char u[4096];
	char splitted1[81];
	concptr splitted2;

	if (!str) return;

	u32b n = strlen(str);
	if (n >= MESSAGE_BUF / 4) return;

	if (n > 80)
	{
#ifdef JP
		concptr t = str;
		for (n = 0; n < 80; n++, t++)
		{
			if (iskanji(*t)) {
				t++;
				n++;
			}
		}

		/* 最後の文字が漢字半分 */
		if (n == 81) n = 79;
#else
		for (n = 80; n > 60; n--)
			if (str[n] == ' ') break;
		if (n == 60) n = 80;
#endif
		splitted2 = str + n;
		strncpy(splitted1, str, n);
		splitted1[n] = '\0';
		str = splitted1;
	}
	else
	{
		splitted2 = NULL;
	}

	m = message_num();
	int k = m / 4;
	if (k > MESSAGE_MAX / 32) k = MESSAGE_MAX / 32;
	for (i = message__next; m; m--)
	{
		int j = 1;
		char buf[1024];
		char *t;
		concptr old;
		if (i-- == 0) i = MESSAGE_MAX - 1;

		old = &message__buf[message__ptr[i]];
		if (!old) continue;

		strcpy(buf, old);
#ifdef JP
		for (t = buf; *t && (*t != '<' || (*(t + 1) != 'x')); t++)
			if (iskanji(*t))t++;
#else
		for (t = buf; *t && (*t != '<'); t++);
#endif
		if (*t)
		{
			if (strlen(buf) < A_MAX) break;

			*(t - 1) = '\0';
			j = atoi(t + 2);
		}

		if (streq(buf, str) && (j < 1000))
		{
			j++;
			message__next = i;
			str = u;
			sprintf(u, "%s <x%d>", buf, j);
			n = strlen(str);
			if (!now_message) now_message++;
		}
		else
		{
			/*流れた行の数を数えておく */
			num_more++;
			now_message++;
		}

		break;
	}

	for (i = message__next; k; k--)
	{
		int q;
		concptr old;

		if (i-- == 0) i = MESSAGE_MAX - 1;

		if (i == message__last) break;

		q = (message__head + MESSAGE_BUF - message__ptr[i]) % MESSAGE_BUF;

		if (q > MESSAGE_BUF / 2) continue;

		old = &message__buf[message__ptr[i]];
		if (!streq(old, str)) continue;

		x = message__next++;
		if (message__next == MESSAGE_MAX) message__next = 0;
		if (message__next == message__last) message__last++;
		if (message__last == MESSAGE_MAX) message__last = 0;

		message__ptr[x] = message__ptr[i];
		if (splitted2 != NULL)
		{
			message_add(splitted2);
		}

		return;
	}

	if (message__head + n + 1 >= MESSAGE_BUF)
	{
		for (i = message__last; TRUE; i++)
		{
			if (i == MESSAGE_MAX) i = 0;
			if (i == message__next) break;
			if (message__ptr[i] >= message__head)
			{
				message__last = i + 1;
			}
		}

		if (message__tail >= message__head) message__tail = 0;

		message__head = 0;
	}

	if (message__head + n + 1 > message__tail)
	{
		message__tail = message__head + n + 1;
		while (message__buf[message__tail - 1]) message__tail++;

		for (i = message__last; TRUE; i++)
		{
			if (i == MESSAGE_MAX) i = 0;
			if (i == message__next) break;
			if ((message__ptr[i] >= message__head) &&
				(message__ptr[i] < message__tail))
			{
				message__last = i + 1;
			}
		}
	}


	x = message__next++;
	if (message__next == MESSAGE_MAX) message__next = 0;
	if (message__next == message__last) message__last++;
	if (message__last == MESSAGE_MAX) message__last = 0;

	message__ptr[x] = message__head;
	for (i = 0; i < n; i++)
	{
		message__buf[message__head + i] = str[i];
	}

	message__buf[message__head + i] = '\0';
	message__head += n + 1;

	if (splitted2 != NULL)
	{
		message_add(splitted2);
	}
}


/*
 * Hack -- flush
 */
static void msg_flush(player_type *player_ptr, int x)
{
	byte a = TERM_L_BLUE;
	bool nagasu = FALSE;
	if ((auto_more && !player_ptr->now_damaged) || num_more < 0) {
		int i;
		for (i = 0; i < 8; i++)
		{
			if (angband_term[i] && (window_flag[i] & PW_MESSAGE)) break;
		}
		if (i < 8)
		{
			if (num_more < angband_term[i]->hgt) nagasu = TRUE;
		}
		else
		{
			nagasu = TRUE;
		}
	}

	player_ptr->now_damaged = FALSE;
	if (!player_ptr->playing || !nagasu)
	{
		Term_putstr(x, 0, -1, a, _("-続く-", "-more-"));
		while (TRUE)
		{
			int cmd = inkey();
			if (cmd == ESCAPE)
			{
				/* auto_moreのとき、全て流す */
				num_more = -9999;
				break;
			}
			else if (cmd == ' ')
			{
				/* 1画面だけ流す */
				num_more = 0;
				break;
			}
			else if ((cmd == '\n') || (cmd == '\r'))
			{
				/* 1行だけ流す */
				num_more--;
				break;
			}

			if (quick_messages) break;
			bell();
		}
	}

	Term_erase(0, 0, 255);
}


void msg_erase(void)
{
	msg_print(NULL);
}


/*
 * todo ここのp_ptrを削除するのは破滅的に作業が増えるので保留
 * Output a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do "Term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to
 * "erase" any "pending" messages still on the screen.
 *
 * Note that we must be very careful about using the
 * "msg_print()" functions without explicitly calling the special
 * "msg_print(NULL)" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * Note that "msg_print(NULL)" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 */
void msg_print(concptr msg)
{
	static int p = 0;
	char *t;
	char buf[1024];

	if (current_world_ptr->timewalk_m_idx) return;

	if (!msg_flag)
	{
		Term_erase(0, 0, 255);
		p = 0;
	}

	int n = (msg ? strlen(msg) : 0);
	if (p && (!msg || ((p + n) > 72)))
	{
		msg_flush(p_ptr, p);
		msg_flag = FALSE;
		p = 0;
	}

	if (!msg) return;
	if (n > 1000) return;

	if (!cheat_turn)
	{
		strcpy(buf, msg);
	}
	else
	{
		sprintf(buf, ("T:%d - %s"), (int)current_world_ptr->game_turn, msg);
	}

	n = strlen(buf);
	if (current_world_ptr->character_generated) message_add(buf);

	t = buf;
	while (n > 72)
	{
		int check, split = 72;
#ifdef JP
		bool k_flag = FALSE;
		int wordlen = 0;
		for (check = 0; check < 72; check++)
		{
			if (k_flag)
			{
				k_flag = FALSE;
				continue;
			}

			if (iskanji(t[check]))
			{
				k_flag = TRUE;
				split = check;
			}
			else if (t[check] == ' ')
			{
				split = check;
				wordlen = 0;
			}
			else
			{
				wordlen++;
				if (wordlen > 20)
					split = check;
			}
		}

#else
		for (check = 40; check < 72; check++)
		{
			if (t[check] == ' ') split = check;
		}
#endif

		char oops = t[split];
		t[split] = '\0';
		Term_putstr(0, 0, split, TERM_WHITE, t);
		msg_flush(p_ptr, split + 1);
		t[split] = oops;
		t[--split] = ' ';
		t += split; n -= split;
	}

	Term_putstr(p, 0, n, TERM_WHITE, t);
	p_ptr->window |= (PW_MESSAGE);
	update_output(p_ptr);

	msg_flag = TRUE;
#ifdef JP
	p += n;
#else
	p += n + 1;
#endif

	if (fresh_message) Term_fresh();
}


void msg_print_wizard(int cheat_type, concptr msg)
{
	if (!cheat_room && cheat_type == CHEAT_DUNGEON) return;
	if (!cheat_peek && cheat_type == CHEAT_OBJECT) return;
	if (!cheat_hear && cheat_type == CHEAT_MONSTER) return;
	if (!cheat_xtra && cheat_type == CHEAT_MISC) return;

	concptr cheat_mes[] = { "ITEM", "MONS", "DUNG", "MISC" };
	char buf[1024];
	sprintf(buf, "WIZ-%s:%s", cheat_mes[cheat_type], msg);
	msg_print(buf);

	if (cheat_diary_output)
	{
		exe_write_diary(p_ptr, DIARY_WIZARD_LOG, 0, buf);
	}

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
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(concptr fmt, ...)
{
	va_list vp;
	char buf[1024];
	va_start(vp, fmt);
	(void)vstrnfmt(buf, 1024, fmt, vp);
	va_end(vp);
	msg_print(buf);
}


/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format_wizard(int cheat_type, concptr fmt, ...)
{
	if (!cheat_room && cheat_type == CHEAT_DUNGEON) return;
	if (!cheat_peek && cheat_type == CHEAT_OBJECT) return;
	if (!cheat_hear && cheat_type == CHEAT_MONSTER) return;
	if (!cheat_xtra && cheat_type == CHEAT_MISC) return;

	va_list vp;
	char buf[1024];
	va_start(vp, fmt);
	(void)vstrnfmt(buf, 1024, fmt, vp);
	va_end(vp);
	msg_print_wizard(cheat_type, buf);
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
 * Get some string input at the cursor location.
 * Assume the buffer is initialized to a default string.
 *
 * The default buffer is in Overwrite mode and displayed in yellow at
 * first.  Normal chars clear the yellow text and append the char in
 * white text.
 *
 * LEFT (^B) and RIGHT (^F) movement keys move the cursor position.
 * If the text is still displayed in yellow (Overwite mode), it will
 * turns into white (Insert mode) when cursor moves.
 *
 * DELETE (^D) deletes a char at the cursor position.
 * BACKSPACE (^H) deletes a char at the left of cursor position.
 * ESCAPE clears the buffer and the window and returns FALSE.
 * RETURN accepts the current buffer contents and returns TRUE.
 */
bool askfor_aux(char *buf, int len, bool numpad_cursor)
{
	/*
	 * Text color
	 * TERM_YELLOW : Overwrite mode
	 * TERM_WHITE : Insert mode
	 */
	byte color = TERM_YELLOW;

	int y, x;
	Term_locate(&x, &y);
	if (len < 1) len = 1;
	if ((x < 0) || (x >= 80)) x = 0;
	if (x + len > 80) len = 80 - x;

	buf[len] = '\0';

	int pos = 0;
	while (TRUE)
	{
		Term_erase(x, y, len);
		Term_putstr(x, y, -1, color, buf);

		Term_gotoxy(x + pos, y);
		int skey = inkey_special(numpad_cursor);

		switch (skey)
		{
		case SKEY_LEFT:
		case KTRL('b'):
		{
			int i = 0;
			color = TERM_WHITE;

			if (0 == pos) break;
			while (TRUE)
			{
				int next_pos = i + 1;
#ifdef JP
				if (iskanji(buf[i])) next_pos++;
#endif
				if (next_pos >= pos) break;

				i = next_pos;
			}

			pos = i;
			break;
		}

		case SKEY_RIGHT:
		case KTRL('f'):
			color = TERM_WHITE;
			if ('\0' == buf[pos]) break;

#ifdef JP
			if (iskanji(buf[pos])) pos += 2;
			else pos++;
#else
			pos++;
#endif
			break;

		case ESCAPE:
			buf[0] = '\0';
			return FALSE;

		case '\n':
		case '\r':
			return TRUE;

		case '\010':
		{
			int i = 0;
			color = TERM_WHITE;
			if (0 == pos) break;
			while (TRUE)
			{
				int next_pos = i + 1;
#ifdef JP
				if (iskanji(buf[i])) next_pos++;
#endif
				if (next_pos >= pos) break;

				i = next_pos;
			}

			pos = i;
		}
			/* Fall through */

		case 0x7F:
		case KTRL('d'):
		{
			color = TERM_WHITE;
			if ('\0' == buf[pos]) break;
			int src = pos + 1;
#ifdef JP
			if (iskanji(buf[pos])) src++;
#endif

			int dst = pos;
			while ('\0' != (buf[dst++] = buf[src++]));
			break;
		}

		default:
		{
			char tmp[100];
			if (skey & SKEY_MASK) break;
			char c = (char)skey;

			if (color == TERM_YELLOW)
			{
				buf[0] = '\0';
				color = TERM_WHITE;
			}

			strcpy(tmp, buf + pos);
#ifdef JP
			if (iskanji(c))
			{
				inkey_base = TRUE;
				char next = inkey();
				if (pos + 1 < len)
				{
					buf[pos++] = c;
					buf[pos++] = next;
				}
				else
				{
					bell();
				}
			}
			else
#endif
			{
#ifdef JP
				if (pos < len && (isprint(c) || iskana(c)))
#else
				if (pos < len && isprint(c))
#endif
				{
					buf[pos++] = c;
				}
				else
				{
					bell();
				}
			}

			buf[pos] = '\0';
			angband_strcat(buf, tmp, len + 1);

			break;
		}
		}

	}
}


/*
 * Get some string input at the cursor location.
 *
 * Allow to use numpad keys as cursor keys.
 */
bool askfor(char *buf, int len)
{
	return askfor_aux(buf, len, TRUE);
}


/*
 * Get a string from the user
 *
 * The "prompt" should take the form "Prompt: "
 *
 * Note that the initial contents of the string is used as
 * the default response, so be sure to "clear" it if needed.
 *
 * We clear the input, and return FALSE, on "ESCAPE".
 */
bool get_string(concptr prompt, char *buf, int len)
{
	bool res;
	msg_print(NULL);
	prt(prompt, 0, 0);
	res = askfor(buf, len);
	prt("", 0, 0);
	return (res);
}


/*
 * Verify something with the user
 *
 * The "prompt" should take the form "Query? "
 *
 * Note that "[y/n]" is appended to the prompt.
 */
bool get_check(concptr prompt)
{
	return get_check_strict(p_ptr, prompt, 0);
}


/*
 * Verify something with the user strictly
 *
 * mode & CHECK_OKAY_CANCEL : force user to answer 'O'kay or 'C'ancel
 * mode & CHECK_NO_ESCAPE   : don't allow ESCAPE key
 * mode & CHECK_NO_HISTORY  : no message_add
 * mode & CHECK_DEFAULT_Y   : accept any key as y, except n and Esc.
 */
bool get_check_strict(player_type *player_ptr, concptr prompt, BIT_FLAGS mode)
{
	char buf[80];
	if (auto_more)
	{
		player_ptr->window |= PW_MESSAGE;
		handle_stuff(player_ptr);
		num_more = 0;
	}

	msg_print(NULL);
	if (!rogue_like_commands)
		mode &= ~CHECK_OKAY_CANCEL;

	if (mode & CHECK_OKAY_CANCEL)
	{
		angband_strcpy(buf, prompt, sizeof(buf) - 15);
		strcat(buf, "[(O)k/(C)ancel]");
	}
	else if (mode & CHECK_DEFAULT_Y)
	{
		angband_strcpy(buf, prompt, sizeof(buf) - 5);
		strcat(buf, "[Y/n]");
	}
	else
	{
		angband_strcpy(buf, prompt, sizeof(buf) - 5);
		strcat(buf, "[y/n]");
	}

	prt(buf, 0, 0);
        if (!(mode & CHECK_NO_HISTORY) && player_ptr->playing)
	{
		message_add(buf);
		player_ptr->window |= (PW_MESSAGE);
		handle_stuff(player_ptr);
	}

	bool flag = FALSE;
	while (TRUE)
	{
		int i = inkey();

		if (!(mode & CHECK_NO_ESCAPE))
		{
			if (i == ESCAPE)
			{
				flag = FALSE;
				break;
			}
		}

		if (mode & CHECK_OKAY_CANCEL)
		{
			if (i == 'o' || i == 'O')
			{
				flag = TRUE;
				break;
			}
			else if (i == 'c' || i == 'C')
			{
				flag = FALSE;
				break;
			}
		}
		else
		{
			if (i == 'y' || i == 'Y')
			{
				flag = TRUE;
				break;
			}
			else if (i == 'n' || i == 'N')
			{
				flag = FALSE;
				break;
			}
		}

		if (mode & CHECK_DEFAULT_Y)
		{
			flag = TRUE;
			break;
		}

		bell();
	}

	prt("", 0, 0);
	return flag;
}


/*
 * Prompts for a keypress
 *
 * The "prompt" should take the form "Command: "
 *
 * Returns TRUE unless the character is "Escape"
 */
bool get_com(concptr prompt, char *command, bool z_escape)
{
	msg_print(NULL);
	prt(prompt, 0, 0);
	if (get_com_no_macros)
		*command = (char)inkey_special(FALSE);
	else
		*command = inkey();

	prt("", 0, 0);
	if (*command == ESCAPE) return FALSE;
	if (z_escape && ((*command == 'z') || (*command == 'Z'))) return FALSE;

	return TRUE;
}


/*
 * Request a "quantity" from the user
 *
 * Hack -- allow "command_arg" to specify a quantity
 */
QUANTITY get_quantity(concptr prompt, QUANTITY max)
{
	bool res;
	char tmp[80];
	char buf[80];

	QUANTITY amt;
	if (command_arg)
	{
		amt = command_arg;
		command_arg = 0;
		if (amt > max) amt = max;

		return (amt);
	}

	COMMAND_CODE code;
	bool result = repeat_pull(&code);
	amt = (QUANTITY)code;
	if ((max != 1) && result)
	{
		if (amt > max) amt = max;
		if (amt < 0) amt = 0;

		return (amt);
	}

	if (!prompt)
	{
		sprintf(tmp, _("いくつですか (1-%d): ", "Quantity (1-%d): "), max);
		prompt = tmp;
	}

	msg_print(NULL);
	prt(prompt, 0, 0);
	amt = 1;
	sprintf(buf, "%d", amt);

	/*
	 * Ask for a quantity
	 * Don't allow to use numpad as cursor key.
	 */
	res = askfor_aux(buf, 6, FALSE);

	prt("", 0, 0);
	if (!res) return 0;

	amt = (COMMAND_CODE)atoi(buf);
	if (isalpha(buf[0])) amt = max;
	if (amt > max) amt = max;
	if (amt < 0) amt = 0;
	if (amt) repeat_push((COMMAND_CODE)amt);

	return (amt);
}


/*
 * Pause for user response
 */
void pause_line(int row)
{
	prt("", row, 0);
	put_str(_("[ 何かキーを押して下さい ]", "[Press any key to continue]"), row, _(26, 23));

	(void)inkey();
	prt("", row, 0);
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


/*
 * Get a keypress from the user.
 * And interpret special keys as internal code.
 *
 * This function is a Mega-Hack and depend on pref-xxx.prf's.
 * Currently works on Linux(UNIX), Windows, and Macintosh only.
 */
int inkey_special(bool numpad_cursor)
{
	static const struct {
		concptr keyname;
		int keyflag;
	} modifier_key_list[] = {
		{"shift-", SKEY_MOD_SHIFT},
		{"control-", SKEY_MOD_CONTROL},
		{NULL, 0},
	};

	static const struct {
		bool numpad;
		concptr keyname;
		int keycode;
	} special_key_list[] = {
		{FALSE, "Down]", SKEY_DOWN},
		{FALSE, "Left]", SKEY_LEFT},
		{FALSE, "Right]", SKEY_RIGHT},
		{FALSE, "Up]", SKEY_UP},
		{FALSE, "Page_Up]", SKEY_PGUP},
		{FALSE, "Page_Down]", SKEY_PGDOWN},
		{FALSE, "Home]", SKEY_TOP},
		{FALSE, "End]", SKEY_BOTTOM},
		{TRUE, "KP_Down]", SKEY_DOWN},
		{TRUE, "KP_Left]", SKEY_LEFT},
		{TRUE, "KP_Right]", SKEY_RIGHT},
		{TRUE, "KP_Up]", SKEY_UP},
		{TRUE, "KP_Page_Up]", SKEY_PGUP},
		{TRUE, "KP_Page_Down]", SKEY_PGDOWN},
		{TRUE, "KP_Home]", SKEY_TOP},
		{TRUE, "KP_End]", SKEY_BOTTOM},
		{TRUE, "KP_2]", SKEY_DOWN},
		{TRUE, "KP_4]", SKEY_LEFT},
		{TRUE, "KP_6]", SKEY_RIGHT},
		{TRUE, "KP_8]", SKEY_UP},
		{TRUE, "KP_9]", SKEY_PGUP},
		{TRUE, "KP_3]", SKEY_PGDOWN},
		{TRUE, "KP_7]", SKEY_TOP},
		{TRUE, "KP_1]", SKEY_BOTTOM},
		{FALSE, NULL, 0},
	};

	static const struct {
		concptr keyname;
		int keycode;
	} gcu_special_key_list[] = {
		{"A", SKEY_UP},
		{"B", SKEY_DOWN},
		{"C", SKEY_RIGHT},
		{"D", SKEY_LEFT},
		{"1~", SKEY_TOP},
		{"4~", SKEY_BOTTOM},
		{"5~", SKEY_PGUP},
		{"6~", SKEY_PGDOWN},
		{NULL, 0},
	};

	char buf[1024];
	concptr str = buf;
	char key;
	int skey = 0;
	int modifier = 0;
	int i;
	size_t trig_len;

	/*
	 * Forget macro trigger ----
	 * It's important if we are already expanding macro action
	 */
	inkey_macro_trigger_string[0] = '\0';

	key = inkey();
	trig_len = strlen(inkey_macro_trigger_string);
	if (!trig_len) return (int)((unsigned char)key);
	if (trig_len == 1 && parse_macro)
	{
		char c = inkey_macro_trigger_string[0];
		forget_macro_action();
		return (int)((unsigned char)c);
	}

	ascii_to_text(buf, inkey_macro_trigger_string);
	if (prefix(str, "\\["))
	{
		str += 2;
		while (TRUE)
		{
			for (i = 0; modifier_key_list[i].keyname; i++)
			{
				if (prefix(str, modifier_key_list[i].keyname))
				{
					str += strlen(modifier_key_list[i].keyname);
					modifier |= modifier_key_list[i].keyflag;
				}
			}

			if (!modifier_key_list[i].keyname) break;
		}

		if (!numpad_as_cursorkey) numpad_cursor = FALSE;

		for (i = 0; special_key_list[i].keyname; i++)
		{
			if ((!special_key_list[i].numpad || numpad_cursor) &&
				streq(str, special_key_list[i].keyname))
			{
				skey = special_key_list[i].keycode;
				break;
			}
		}

		if (skey)
		{
			forget_macro_action();
			return (skey | modifier);
		}
	}

	if (prefix(str, "\\e["))
	{
		str += 3;

		for (i = 0; gcu_special_key_list[i].keyname; i++)
		{
			if (streq(str, gcu_special_key_list[i].keyname))
			{
				return gcu_special_key_list[i].keycode;
			}
		}
	}

	inkey_macro_trigger_string[0] = '\0';
	return (int)((unsigned char)key);
}
