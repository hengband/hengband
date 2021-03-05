﻿#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "player/player-race-types.h"
#include "player/process-name.h"
#include "racial/racial-android.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "world/world.h"

/*!
 * @brief 画面を再描画するコマンドのメインルーチン
 * Hack -- redraw the screen
 * @param creature_ptr プレーヤーへの参照ポインタ
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
void do_cmd_redraw(player_type *creature_ptr)
{
	term_xtra(TERM_XTRA_REACT, 0);

	creature_ptr->update |= (PU_COMBINE | PU_REORDER);
	creature_ptr->update |= (PU_TORCH);
	creature_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
	creature_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
	creature_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
	creature_ptr->update |= (PU_MONSTERS);

	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

	creature_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
	creature_ptr->window_flags |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

	update_playtime();
	handle_stuff(creature_ptr);
	if (creature_ptr->prace == RACE_ANDROID) calc_android_exp(creature_ptr);

	term_type *old = Term;
	for (int j = 0; j < 8; j++)
	{
		if (!angband_term[j]) continue;

		term_activate(angband_term[j]);
		term_redraw();
		term_fresh();
		term_activate(old);
	}
}


/*!
 * @brief プレイヤーのステータス表示
 * @return なし
 */
void do_cmd_player_status(player_type *creature_ptr)
{
	int mode = 0;
	char tmp[160];
	screen_save();
	while (TRUE)
	{
		update_playtime();
		display_player(creature_ptr, mode);

		if (mode == 4)
		{
			mode = 0;
			display_player(creature_ptr, mode);
		}

		term_putstr(2, 23, -1, TERM_WHITE,
			_("['c'で名前変更, 'f'でファイルへ書出, 'h'でモード変更, ESCで終了]", "['c' to change name, 'f' to file, 'h' to change mode, or ESC]"));
		char c = inkey();
		if (c == ESCAPE) break;

		if (c == 'c')
		{
			get_name(creature_ptr);
			process_player_name(creature_ptr, FALSE);
		}
		else if (c == 'f')
		{
			sprintf(tmp, "%s.txt", creature_ptr->base_name);
			if (get_string(_("ファイル名: ", "File name: "), tmp, 80))
			{
				if (tmp[0] && (tmp[0] != ' '))
				{
					file_character(creature_ptr, tmp, update_playtime, display_player);
				}
			}
		}
		else if (c == 'h')
		{
			mode++;
		}
		else
		{
			bell();
		}

		msg_erase();
	}

	screen_load();
	creature_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);
	handle_stuff(creature_ptr);
}


/*!
 * @brief 最近表示されたメッセージを再表示するコマンドのメインルーチン
 * Recall the most recent message
 * @return なし
 */
void do_cmd_message_one(void)
{
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
	char shower_str[81];
	char finder_str[81];
	char back_str[81];
	concptr shower = NULL;
	int wid, hgt;
	term_get_size(&wid, &hgt);
	int num_lines = hgt - 4;
	strcpy(finder_str, "");
	strcpy(shower_str, "");
	int n = message_num();
	int i = 0;
	screen_save();
	term_clear();
	while (TRUE)
	{
		int j;
		int skey;
		for (j = 0; (j < num_lines) && (i + j < n); j++)
		{
			concptr msg = message_str(i + j);
			c_prt((i + j < num_now ? TERM_WHITE : TERM_SLATE), msg, num_lines + 1 - j, 0);
			if (!shower || !shower[0]) continue;

			concptr str = msg;
			while ((str = angband_strstr(str, shower)) != NULL)
			{
				int len = strlen(shower);
				term_putstr(str - msg, num_lines + 1 - j, len, TERM_YELLOW, shower);
				str += len;
			}
		}

		for (; j < num_lines; j++)
			term_erase(0, num_lines + 1 - j, 255);

		prt(format(_("以前のメッセージ %d-%d 全部で(%d)", "Message Recall (%d-%d of %d)"),
			i, i + j - 1, n), 0, 0);
		prt(_("[ 'p' で更に古いもの, 'n' で更に新しいもの, '/' で検索, ESC で中断 ]",
			"[Press 'p' for older, 'n' for newer, ..., or ESCAPE]"), hgt - 1, 0);
		skey = inkey_special(TRUE);
		if (skey == ESCAPE) break;

		j = i;
		switch (skey)
		{
		case '=':
			prt(_("強調: ", "Show: "), hgt - 1, 0);
			strcpy(back_str, shower_str);
			if (askfor(shower_str, 80))
				shower = shower_str[0] ? shower_str : NULL;
			else
				strcpy(shower_str, back_str);

			continue;
		case '/':
		case KTRL('s'):
		{
			prt(_("検索: ", "Find: "), hgt - 1, 0);
			strcpy(back_str, finder_str);
			if (!askfor(finder_str, 80))
			{
				strcpy(finder_str, back_str);
				continue;
			}
			else if (!finder_str[0])
			{
				shower = NULL;
				continue;
			}

			shower = finder_str;
			for (int z = i + 1; z < n; z++)
			{
				concptr msg = message_str(z);
				if (angband_strstr(msg, finder_str))
				{
					i = z;
					break;
				}
			}
		}

		break;

		case SKEY_TOP:
			i = n - num_lines;
			break;
		case SKEY_BOTTOM:
			i = 0;
			break;
		case '8':
		case SKEY_UP:
		case '\n':
		case '\r':
			i = MIN(i + 1, n - num_lines);
			break;
		case '+':
			i = MIN(i + 10, n - num_lines);
			break;
		case 'p':
		case KTRL('P'):
		case ' ':
		case SKEY_PGUP:
			i = MIN(i + num_lines, n - num_lines);
			break;
		case 'n':
		case KTRL('N'):
		case SKEY_PGDOWN:
			i = MAX(0, i - num_lines);
			break;
		case '-':
			i = MAX(0, i - 10);
			break;
		case '2':
		case SKEY_DOWN:
			i = MAX(0, i - 1);
			break;
		}

		if (i == j) bell();
	}

	screen_load();
}
