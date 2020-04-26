/*!
 * @file autopick.c
 * @brief 自動拾い機能の実装 / Object Auto-picker/Destroyer
 * @date 2014/01/02
 * @author
 * Copyright (c) 2002  Mogami\n
 *\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"
#include "util.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-key-flag-process.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-destroyer.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-searcher.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-registry.h"
#include "gameterm.h"
#include "autopick/autopick.h"
#include "core/show-file.h"
#include "cmd/cmd-save.h"
#include "io/read-pref-file.h"

#include "mind.h"

#include "market/store.h"
#include "player-move.h"
#include "player-class.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "object-flavor.h"

#include "world.h"
#include "view/display-main-window.h" // 暫定。後で消す.

/*
 *  Auto-destroy marked item
 */
static void autopick_delayed_alter_aux(player_type *player_ptr, INVENTORY_IDX item)
{
	object_type *o_ptr;
	o_ptr = REF_ITEM(player_ptr, player_ptr->current_floor_ptr, item);

	if (o_ptr->k_idx == 0 || !(o_ptr->marked & OM_AUTODESTROY)) return;

	GAME_TEXT o_name[MAX_NLEN];
	object_desc(player_ptr, o_name, o_ptr, 0);
	if (item >= 0)
	{
		inven_item_increase(player_ptr, item, -(o_ptr->number));
		inven_item_optimize(player_ptr, item);
	}
	else
	{
		delete_object_idx(player_ptr, 0 - item);
	}

	msg_format(_("%sを自動破壊します。", "Auto-destroying %s."), o_name);
}


/*
 *  Auto-destroy marked items in inventry and on floor
 */
void autopick_delayed_alter(player_type *owner_ptr)
{
	INVENTORY_IDX item;

	/*
	 * Scan inventry in reverse order to prevent
	 * skipping after inven_item_optimize()
	 */
	for (item = INVEN_TOTAL - 1; item >= 0; item--)
		autopick_delayed_alter_aux(owner_ptr, item);

	floor_type *floor_ptr = owner_ptr->current_floor_ptr;
	item = floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx;
	while (item)
	{
		OBJECT_IDX next = floor_ptr->o_list[item].next_o_idx;
		autopick_delayed_alter_aux(owner_ptr, -item);
		item = next;
	}
}


/*
 * Auto-inscription and/or destroy
 *
 * Auto-destroyer works only on inventory or on floor stack only when
 * requested.
 */
void autopick_alter_item(player_type *player_ptr, INVENTORY_IDX item, bool destroy)
{
	object_type *o_ptr;
	o_ptr = REF_ITEM(player_ptr, player_ptr->current_floor_ptr, item);
	int idx = find_autopick_list(player_ptr, o_ptr);
	auto_inscribe_item(player_ptr, o_ptr, idx);
	if (destroy && item <= INVEN_PACK)
		auto_destroy_item(player_ptr, o_ptr, idx);
}


/*
 * Automatically pickup/destroy items in this grid.
 */
void autopick_pickup_items(player_type* player_ptr, grid_type *g_ptr)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		int idx = find_autopick_list(player_ptr, o_ptr);
		auto_inscribe_item(player_ptr, o_ptr, idx);
		bool is_auto_pickup = idx >= 0;
		is_auto_pickup &= (autopick_list[idx].action & (DO_AUTOPICK | DO_QUERY_AUTOPICK)) != 0;
		if (!is_auto_pickup)
		{
			auto_destroy_item(player_ptr, o_ptr, idx);
			continue;
		}

		disturb(player_ptr, FALSE, FALSE);
		if (!inven_carry_okay(o_ptr))
		{
			GAME_TEXT o_name[MAX_NLEN];
			object_desc(player_ptr, o_name, o_ptr, 0);
			msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
			o_ptr->marked |= OM_NOMSG;
			continue;
		}

		if (!(autopick_list[idx].action & DO_QUERY_AUTOPICK))
		{
			py_pickup_aux(player_ptr, this_o_idx);
			continue;
		}

		char out_val[MAX_NLEN + 20];
		GAME_TEXT o_name[MAX_NLEN];
		if (o_ptr->marked & OM_NO_QUERY)
		{
			continue;
		}

		object_desc(player_ptr, o_name, o_ptr, 0);
		sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
		if (!get_check(out_val))
		{
			o_ptr->marked |= OM_NOMSG | OM_NO_QUERY;
			continue;
		}

		py_pickup_aux(player_ptr, this_o_idx);
	}
}


/*
 * Delete or insert string
 */
static void toggle_keyword(text_body_type *tb, BIT_FLAGS flg)
{
	int by1, by2;
	bool add = TRUE;
	bool fixed = FALSE;
	if (tb->mark)
	{
		by1 = MIN(tb->my, tb->cy);
		by2 = MAX(tb->my, tb->cy);
	}
	else /* if (!tb->mark) */
	{
		by1 = by2 = tb->cy;
	}

	for (int y = by1; y <= by2; y++)
	{
		autopick_type an_entry, *entry = &an_entry;
		if (!autopick_new_entry(entry, tb->lines_list[y], !fixed)) continue;

		string_free(tb->lines_list[y]);
		if (!fixed)
		{
			if (!IS_FLG(flg)) add = TRUE;
			else add = FALSE;

			fixed = TRUE;
		}

		if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END)
		{
			int i;
			for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++)
				REM_FLG(i);
		}
		else if (FLG_UNAWARE <= flg && flg <= FLG_STAR_IDENTIFIED)
		{
			int i;
			for (i = FLG_UNAWARE; i <= FLG_STAR_IDENTIFIED; i++)
				REM_FLG(i);
		}
		else if (FLG_ARTIFACT <= flg && flg <= FLG_AVERAGE)
		{
			int i;
			for (i = FLG_ARTIFACT; i <= FLG_AVERAGE; i++)
				REM_FLG(i);
		}
		else if (FLG_RARE <= flg && flg <= FLG_COMMON)
		{
			int i;
			for (i = FLG_RARE; i <= FLG_COMMON; i++)
				REM_FLG(i);
		}

		if (add) ADD_FLG(flg);
		else REM_FLG(flg);

		tb->lines_list[y] = autopick_line_from_entry_kill(entry);
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
	}
}


/*
 * Change command letter
 */
static void toggle_command_letter(text_body_type *tb, byte flg)
{
	autopick_type an_entry;
	autopick_type *entry = &an_entry;
	int by1, by2;
	bool add = TRUE;
	bool fixed = FALSE;
	if (tb->mark)
	{
		by1 = MIN(tb->my, tb->cy);
		by2 = MAX(tb->my, tb->cy);
	}
	else /* if (!tb->mark) */
	{
		by1 = by2 = tb->cy;
	}

	for (int y = by1; y <= by2; y++)
	{
		int wid = 0;

		if (!autopick_new_entry(entry, tb->lines_list[y], FALSE)) continue;

		string_free(tb->lines_list[y]);

		if (!fixed)
		{
			if (!(entry->action & flg)) add = TRUE;
			else add = FALSE;

			fixed = TRUE;
		}

		if (entry->action & DONT_AUTOPICK) wid--;
		else if (entry->action & DO_AUTODESTROY) wid--;
		else if (entry->action & DO_QUERY_AUTOPICK) wid--;
		if (!(entry->action & DO_DISPLAY)) wid--;

		if (flg != DO_DISPLAY)
		{
			entry->action &= ~(DO_AUTOPICK | DONT_AUTOPICK | DO_AUTODESTROY | DO_QUERY_AUTOPICK);
			if (add) entry->action |= flg;
			else entry->action |= DO_AUTOPICK;
		}
		else
		{
			entry->action &= ~(DO_DISPLAY);
			if (add) entry->action |= flg;
		}

		if (tb->cy == y)
		{
			if (entry->action & DONT_AUTOPICK) wid++;
			else if (entry->action & DO_AUTODESTROY) wid++;
			else if (entry->action & DO_QUERY_AUTOPICK) wid++;
			if (!(entry->action & DO_DISPLAY)) wid++;

			if (wid > 0) tb->cx++;
			if (wid < 0 && tb->cx > 0) tb->cx--;
		}

		tb->lines_list[y] = autopick_line_from_entry_kill(entry);
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
	}
}


/*
 * Delete or insert string
 */
static void add_keyword(text_body_type *tb, BIT_FLAGS flg)
{
	int by1, by2;
	if (tb->mark)
	{
		by1 = MIN(tb->my, tb->cy);
		by2 = MAX(tb->my, tb->cy);
	}
	else
	{
		by1 = by2 = tb->cy;
	}

	for (int y = by1; y <= by2; y++)
	{
		autopick_type an_entry, *entry = &an_entry;
		if (!autopick_new_entry(entry, tb->lines_list[y], FALSE)) continue;

		if (IS_FLG(flg))
		{
			autopick_free_entry(entry);
			continue;
		}

		string_free(tb->lines_list[y]);
		if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END)
		{
			int i;
			for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++)
				REM_FLG(i);
		}

		ADD_FLG(flg);
		tb->lines_list[y] = autopick_line_from_entry_kill(entry);
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
	}
}


/*
 * Add an empty line at the last of the file
 */
static bool add_empty_line(text_body_type *tb)
{
	int num_lines;
	for (num_lines = 0; tb->lines_list[num_lines]; num_lines++);

	if (num_lines >= MAX_LINES - 2) return FALSE;
	if (!tb->lines_list[num_lines - 1][0]) return FALSE;

	tb->lines_list[num_lines] = string_make("");
	tb->dirty_flags |= DIRTY_EXPRESSION;
	tb->changed = TRUE;
	return TRUE;
}


/*
 * Display the menu, and get a command
 */
static int do_command_menu(int level, int start)
{
	int max_len = 0;
	int col0 = 5 + level * 7;
	int row0 = 1 + level * 3;
	int menu_id_list[26];
	bool redraw = TRUE;
	char linestr[MAX_LINELEN];

	byte menu_key = 0;
	for (int i = start; menu_data[i].level >= level; i++)
	{
		/* Ignore lower level sub menus */
		if (menu_data[i].level > level) continue;

		int len = strlen(menu_data[i].name);
		if (len > max_len) max_len = len;

		menu_id_list[menu_key] = i;
		menu_key++;
	}

	while (menu_key < 26)
	{
		menu_id_list[menu_key] = -1;
		menu_key++;
	}

	int max_menu_wid = max_len + 3 + 3;

	/* Prepare box line */
	linestr[0] = '\0';
	strcat(linestr, "+");
	for (int i = 0; i < max_menu_wid + 2; i++)
	{
		strcat(linestr, "-");
	}

	strcat(linestr, "+");

	while (TRUE)
	{
		if (redraw)
		{
			int row1 = row0 + 1;
			Term_putstr(col0, row0, -1, TERM_WHITE, linestr);

			menu_key = 0;
			for (int i = start; menu_data[i].level >= level; i++)
			{
				char com_key_str[3];
				concptr str;
				if (menu_data[i].level > level) continue;

				if (menu_data[i].com_id == -1)
				{
					strcpy(com_key_str, _("▼", ">"));
				}
				else if (menu_data[i].key != -1)
				{
					com_key_str[0] = '^';
					com_key_str[1] = menu_data[i].key + '@';
					com_key_str[2] = '\0';
				}
				else
				{
					com_key_str[0] = '\0';
				}

				str = format("| %c) %-*s %2s | ", menu_key + 'a', max_len, menu_data[i].name, com_key_str);

				Term_putstr(col0, row1++, -1, TERM_WHITE, str);

				menu_key++;
			}

			Term_putstr(col0, row1, -1, TERM_WHITE, linestr);
			redraw = FALSE;
		}

		prt(format(_("(a-%c) コマンド:", "(a-%c) Command:"), menu_key + 'a' - 1), 0, 0);
		char key = inkey();

		if (key == ESCAPE) return 0;

		int com_id;
		bool is_alphabet = key >= 'a' && key <= 'z';
		if (!is_alphabet)
		{
			com_id = get_com_id(key);
			if (com_id)
			{
				return com_id;
			}

			continue;
		}

		int menu_id = menu_id_list[key - 'a'];

		if (menu_id < 0) continue;

		com_id = menu_data[menu_id].com_id;

		if (com_id == -1)
		{
			com_id = do_command_menu(level + 1, menu_id + 1);

			if (com_id) return com_id;
			else redraw = TRUE;
		}
		else if (com_id)
		{
			return com_id;
		}
	}
}


static chain_str_type *new_chain_str(concptr str)
{
	chain_str_type *chain;
	size_t len = strlen(str);
	chain = (chain_str_type *)ralloc(sizeof(chain_str_type) + len * sizeof(char));
	strcpy(chain->s, str);
	chain->next = NULL;
	return chain;
}


static void kill_yank_chain(text_body_type *tb)
{
	chain_str_type *chain = tb->yank;
	tb->yank = NULL;
	tb->yank_eol = TRUE;

	while (chain)
	{
		chain_str_type *next = chain->next;
		size_t len = strlen(chain->s);

		rnfree(chain, sizeof(chain_str_type) + len * sizeof(char));

		chain = next;
	}
}


static void add_str_to_yank(text_body_type *tb, concptr str)
{
	tb->yank_eol = FALSE;
	if (NULL == tb->yank)
	{
		tb->yank = new_chain_str(str);
		return;
	}

	chain_str_type *chain;
	chain = tb->yank;

	while (TRUE)
	{
		if (!chain->next)
		{
			chain->next = new_chain_str(str);
			return;
		}

		/* Go to next */
		chain = chain->next;
	}
}


/*
 * Do work for the copy editor-command
 */
static void copy_text_to_yank(text_body_type *tb)
{
	int len = strlen(tb->lines_list[tb->cy]);
	if (tb->cx > len) tb->cx = len;

	if (!tb->mark)
	{
		tb->cx = 0;
		tb->my = tb->cy;
		tb->mx = len;
	}

	kill_yank_chain(tb);
	if (tb->my != tb->cy)
	{
		int by1 = MIN(tb->my, tb->cy);
		int by2 = MAX(tb->my, tb->cy);

		for (int y = by1; y <= by2; y++)
		{
			add_str_to_yank(tb, tb->lines_list[y]);
		}

		add_str_to_yank(tb, "");
		tb->mark = 0;
		tb->dirty_flags |= DIRTY_ALL;
		return;
	}

	char buf[MAX_LINELEN];
	int bx1 = MIN(tb->mx, tb->cx);
	int bx2 = MAX(tb->mx, tb->cx);
	if (bx2 > len) bx2 = len;

	if (bx1 == 0 && bx2 == len)
	{
		add_str_to_yank(tb, tb->lines_list[tb->cy]);
		add_str_to_yank(tb, "");
	}
	else
	{
		int end = bx2 - bx1;
		for (int i = 0; i < bx2 - bx1; i++)
		{
			buf[i] = tb->lines_list[tb->cy][bx1 + i];
		}

		buf[end] = '\0';
		add_str_to_yank(tb, buf);
	}

	tb->mark = 0;
	tb->dirty_flags |= DIRTY_ALL;
}


/*
 * Execute a single editor command
 */
static bool do_editor_command(player_type *player_ptr, text_body_type *tb, int com_id)
{
	switch (com_id)
	{
	case EC_QUIT:
		if (tb->changed)
		{
			if (!get_check(_("全ての変更を破棄してから終了します。よろしいですか？ ",
				"Discard all changes and quit. Are you sure? "))) break;
		}

		return QUIT_WITHOUT_SAVE;

	case EC_SAVEQUIT:
		return QUIT_AND_SAVE;

	case EC_REVERT:
		if (!get_check(_("全ての変更を破棄して元の状態に戻します。よろしいですか？ ",
			"Discard all changes and revert to original file. Are you sure? "))) break;

		free_text_lines(tb->lines_list);
		tb->lines_list = read_pickpref_text_lines(player_ptr, &tb->filename_mode);
		tb->dirty_flags |= DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
		tb->cx = tb->cy = 0;
		tb->mark = 0;

		tb->changed = FALSE;
		break;

	case EC_HELP:
		(void)show_file(player_ptr, TRUE, _("jeditor.txt", "editor.txt"), NULL, 0, 0);
		tb->dirty_flags |= DIRTY_SCREEN;

		break;

	case EC_RETURN:
		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

		insert_return_code(tb);
		tb->cy++;
		tb->cx = 0;

		tb->dirty_flags |= DIRTY_ALL;
		break;

	case EC_LEFT:
	{
		if (0 < tb->cx)
		{
			int len;
#ifdef JP
			int i;
#endif
			tb->cx--;
			len = strlen(tb->lines_list[tb->cy]);
			if (len < tb->cx) tb->cx = len;
#ifdef JP
			for (i = 0; tb->lines_list[tb->cy][i]; i++)
			{
				if (iskanji(tb->lines_list[tb->cy][i]))
				{
					i++;
					if (i == tb->cx)
					{
						tb->cx--;
						break;
					}
				}
			}
#endif
		}
		else if (tb->cy > 0)
		{
			tb->cy--;
			tb->cx = strlen(tb->lines_list[tb->cy]);
		}

		break;
	}
	case EC_DOWN:
		if (!tb->lines_list[tb->cy + 1])
		{
			if (!add_empty_line(tb)) break;
		}

		tb->cy++;
		break;

	case EC_UP:
		if (tb->cy > 0) tb->cy--;
		break;

	case EC_RIGHT:
	{
#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][tb->cx])) tb->cx++;
#endif
		tb->cx++;
		int len = strlen(tb->lines_list[tb->cy]);
		if (len < tb->cx)
		{
			tb->cx = len;
			if (!tb->lines_list[tb->cy + 1])
			{
				if (!add_empty_line(tb)) break;
			}

			tb->cy++;
			tb->cx = 0;
		}

		break;
	}
	case EC_BOL:
		tb->cx = 0;
		break;

	case EC_EOL:
		tb->cx = strlen(tb->lines_list[tb->cy]);
		break;

	case EC_PGUP:
		while (0 < tb->cy && tb->upper <= tb->cy)
			tb->cy--;
		while (0 < tb->upper && tb->cy + 1 < tb->upper + tb->hgt)
			tb->upper--;
		break;

	case EC_PGDOWN:
		while (tb->cy < tb->upper + tb->hgt)
		{
			if (!tb->lines_list[tb->cy + 1])
			{
				if (!add_empty_line(tb)) break;
			}

			tb->cy++;
		}

		tb->upper = tb->cy;
		break;

	case EC_TOP:
		tb->cy = 0;
		break;

	case EC_BOTTOM:
		while (TRUE)
		{
			if (!tb->lines_list[tb->cy + 1])
			{
				if (!add_empty_line(tb)) break;
			}

			tb->cy++;
		}

		tb->cx = 0;
		break;

	case EC_CUT:
	{
		copy_text_to_yank(tb);
		if (tb->my == tb->cy)
		{
			int bx1 = MIN(tb->mx, tb->cx);
			int bx2 = MAX(tb->mx, tb->cx);
			int len = strlen(tb->lines_list[tb->cy]);
			if (bx2 > len) bx2 = len;

			kill_line_segment(tb, tb->cy, bx1, bx2, TRUE);
			tb->cx = bx1;
		}
		else
		{
			int by1 = MIN(tb->my, tb->cy);
			int by2 = MAX(tb->my, tb->cy);

			for (int y = by2; y >= by1; y--)
			{
				int len = strlen(tb->lines_list[y]);

				kill_line_segment(tb, y, 0, len, TRUE);
			}

			tb->cy = by1;
			tb->cx = 0;
		}

		tb->mark = 0;
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
		break;
	}
	case EC_COPY:
		copy_text_to_yank(tb);

		/*
		 * Move cursor position to the end of the selection
		 *
		 * Pressing ^C ^V correctly duplicates the selection.
		 */
		if (tb->my != tb->cy)
		{
			tb->cy = MAX(tb->cy, tb->my);
			if (!tb->lines_list[tb->cy + 1])
			{
				if (!add_empty_line(tb)) break;
			}

			tb->cy++;
			break;
		}

		tb->cx = MAX(tb->cx, tb->mx);
		if (!tb->lines_list[tb->cy][tb->cx])
		{
			if (!tb->lines_list[tb->cy + 1])
			{
				if (!add_empty_line(tb)) break;
			}

			tb->cy++;
			tb->cx = 0;
		}

		break;

	case EC_PASTE:
	{
		chain_str_type *chain = tb->yank;
		int len = strlen(tb->lines_list[tb->cy]);
		if (!chain) break;
		if (tb->cx > len) tb->cx = len;

		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

		while (chain)
		{
			concptr yank_str = chain->s;
			char buf[MAX_LINELEN];
			int i;
			char rest[MAX_LINELEN], *rest_ptr = rest;
			for (i = 0; i < tb->cx; i++)
				buf[i] = tb->lines_list[tb->cy][i];

			strcpy(rest, &(tb->lines_list[tb->cy][i]));
			while (*yank_str && i < MAX_LINELEN - 1)
			{
				buf[i++] = *yank_str++;
			}

			buf[i] = '\0';
			chain = chain->next;
			if (chain || tb->yank_eol)
			{
				insert_return_code(tb);
				string_free(tb->lines_list[tb->cy]);
				tb->lines_list[tb->cy] = string_make(buf);
				tb->cx = 0;
				tb->cy++;

				continue;
			}

			tb->cx = strlen(buf);
			while (*rest_ptr && i < MAX_LINELEN - 1)
			{
				buf[i++] = *rest_ptr++;
			}

			buf[i] = '\0';
			string_free(tb->lines_list[tb->cy]);
			tb->lines_list[tb->cy] = string_make(buf);
			break;
		}

		tb->dirty_flags |= DIRTY_ALL;
		tb->dirty_flags |= DIRTY_EXPRESSION;
		tb->changed = TRUE;
		break;
	}
	case EC_BLOCK:
	{
		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
			break;
		}

		tb->mark = MARK_MARK;
		if (com_id == tb->old_com_id)
		{
			int tmp = tb->cy;
			tb->cy = tb->my;
			tb->my = tmp;
			tmp = tb->cx;
			tb->cx = tb->mx;
			tb->mx = tmp;
			tb->dirty_flags |= DIRTY_ALL;
			break;
		}

		int len = strlen(tb->lines_list[tb->cy]);

		tb->my = tb->cy;
		tb->mx = tb->cx;
		if (tb->cx > len) tb->mx = len;
		break;
	}
	case EC_KILL_LINE:
	{
		int len = strlen(tb->lines_list[tb->cy]);
		if (tb->cx > len) tb->cx = len;

		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

		if (tb->old_com_id != com_id)
		{
			kill_yank_chain(tb);
			tb->yank = NULL;
		}

		if (tb->cx < len)
		{
			add_str_to_yank(tb, &(tb->lines_list[tb->cy][tb->cx]));
			kill_line_segment(tb, tb->cy, tb->cx, len, FALSE);
			tb->dirty_line = tb->cy;
			break;
		}

		if (tb->yank_eol) add_str_to_yank(tb, "");

		tb->yank_eol = TRUE;
		do_editor_command(player_ptr, tb, EC_DELETE_CHAR);
		break;
	}
	case EC_DELETE_CHAR:
	{
		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][tb->cx])) tb->cx++;
#endif
		tb->cx++;
		int len = strlen(tb->lines_list[tb->cy]);
		if (len >= tb->cx)
		{
			do_editor_command(player_ptr, tb, EC_BACKSPACE);
			break;
		}

		if (tb->lines_list[tb->cy + 1])
		{
			tb->cy++;
			tb->cx = 0;
		}
		else
		{
			tb->cx = len;
			break;
		}

		do_editor_command(player_ptr, tb, EC_BACKSPACE);
		break;
	}
	case EC_BACKSPACE:
	{
		int len, i, j, k;
		char buf[MAX_LINELEN];
		if (tb->mark)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

		len = strlen(tb->lines_list[tb->cy]);
		if (len < tb->cx) tb->cx = len;

		if (tb->cx == 0)
		{
			if (tb->cy == 0) break;
			tb->cx = strlen(tb->lines_list[tb->cy - 1]);
			strcpy(buf, tb->lines_list[tb->cy - 1]);
			strcat(buf, tb->lines_list[tb->cy]);
			string_free(tb->lines_list[tb->cy - 1]);
			string_free(tb->lines_list[tb->cy]);
			tb->lines_list[tb->cy - 1] = string_make(buf);

			for (i = tb->cy; tb->lines_list[i + 1]; i++)
				tb->lines_list[i] = tb->lines_list[i + 1];

			tb->lines_list[i] = NULL;
			tb->cy--;
			tb->dirty_flags |= DIRTY_ALL;
			tb->dirty_flags |= DIRTY_EXPRESSION;
			tb->changed = TRUE;
			break;
		}

		for (i = j = k = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
		{
			k = j;
#ifdef JP
			if (iskanji(tb->lines_list[tb->cy][i]))
				buf[j++] = tb->lines_list[tb->cy][i++];
#endif
			buf[j++] = tb->lines_list[tb->cy][i];
		}

		while (j > k)
		{
			tb->cx--;
			j--;
		}

		for (; tb->lines_list[tb->cy][i]; i++)
		{
			buf[j++] = tb->lines_list[tb->cy][i];
		}

		buf[j] = '\0';
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make(buf);
		tb->dirty_line = tb->cy;
		check_expression_line(tb, tb->cy);
		tb->changed = TRUE;
		break;
	}
	case EC_SEARCH_STR:
	{
		byte search_dir;
		tb->dirty_flags |= DIRTY_SCREEN;
		search_dir = get_string_for_search(player_ptr, &tb->search_o_ptr, &tb->search_str);

		if (!search_dir) break;

		if (search_dir == 1) do_editor_command(player_ptr, tb, EC_SEARCH_FORW);
		else do_editor_command(player_ptr, tb, EC_SEARCH_BACK);
		break;
	}
	case EC_SEARCH_FORW:
		if (tb->search_o_ptr)
		{
			search_for_object(player_ptr, tb, tb->search_o_ptr, TRUE);
			break;
		}

		if (tb->search_str && tb->search_str[0])
		{
			search_for_string(tb, tb->search_str, TRUE);
			break;
		}

		tb->dirty_flags |= DIRTY_NO_SEARCH;
		break;

	case EC_SEARCH_BACK:
		if (tb->search_o_ptr)
		{
			search_for_object(player_ptr, tb, tb->search_o_ptr, FALSE);
			break;
		}

		if (tb->search_str && tb->search_str[0])
		{
			search_for_string(tb, tb->search_str, FALSE);
			break;
		}

		tb->dirty_flags |= DIRTY_NO_SEARCH;
		break;

	case EC_SEARCH_OBJ:
		tb->dirty_flags |= DIRTY_SCREEN;

		if (!get_object_for_search(player_ptr, &tb->search_o_ptr, &tb->search_str)) break;

		do_editor_command(player_ptr, tb, EC_SEARCH_FORW);
		break;

	case EC_SEARCH_DESTROYED:
		if (!get_destroyed_object_for_search(player_ptr, &tb->search_o_ptr, &tb->search_str))
		{
			tb->dirty_flags |= DIRTY_NO_SEARCH;
			break;
		}

		do_editor_command(player_ptr, tb, EC_SEARCH_FORW);
		break;

	case EC_INSERT_OBJECT:
	{
		autopick_type an_entry, *entry = &an_entry;
		if (!entry_from_choosed_object(player_ptr, entry))
		{
			tb->dirty_flags |= DIRTY_SCREEN;
			break;
		}

		tb->cx = 0;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = autopick_line_from_entry_kill(entry);
		tb->dirty_flags |= DIRTY_SCREEN;
		break;
	}
	case EC_INSERT_DESTROYED:
		if (!tb->last_destroyed) break;

		tb->cx = 0;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make(tb->last_destroyed);
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
		break;

	case EC_INSERT_BLOCK:
	{
		char expression[80];
		sprintf(expression, "?:[AND [EQU $RACE %s] [EQU $CLASS %s] [GEQ $LEVEL %02d]]",
#ifdef JP
			rp_ptr->E_title, cp_ptr->E_title,
#else
			rp_ptr->title, cp_ptr->title,
#endif
			player_ptr->lev);
		tb->cx = 0;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make(expression);
		tb->cy++;
		insert_return_code(tb);
		string_free(tb->lines_list[tb->cy]);
		tb->lines_list[tb->cy] = string_make("?:1");
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
		break;
	}

	case EC_INSERT_MACRO:
		draw_text_editor(player_ptr, tb);
		Term_erase(0, tb->cy - tb->upper + 1, tb->wid);
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, _("P:<トリガーキー>: ", "P:<Trigger key>: "));
		if (!insert_macro_line(tb)) break;

		tb->cx = 2;
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
		break;

	case EC_INSERT_KEYMAP:
		draw_text_editor(player_ptr, tb);
		Term_erase(0, tb->cy - tb->upper + 1, tb->wid);
		Term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW,
			format(_("C:%d:<コマンドキー>: ", "C:%d:<Keypress>: "), (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));

		if (!insert_keymap_line(tb)) break;

		tb->cx = 2;
		tb->dirty_flags |= DIRTY_ALL;
		tb->changed = TRUE;
		break;

	case EC_CL_AUTOPICK: toggle_command_letter(tb, DO_AUTOPICK); break;
	case EC_CL_DESTROY: toggle_command_letter(tb, DO_AUTODESTROY); break;
	case EC_CL_LEAVE: toggle_command_letter(tb, DONT_AUTOPICK); break;
	case EC_CL_QUERY: toggle_command_letter(tb, DO_QUERY_AUTOPICK); break;
	case EC_CL_NO_DISP: toggle_command_letter(tb, DO_DISPLAY); break;

	case EC_IK_UNAWARE: toggle_keyword(tb, FLG_UNAWARE); break;
	case EC_IK_UNIDENTIFIED: toggle_keyword(tb, FLG_UNIDENTIFIED); break;
	case EC_IK_IDENTIFIED: toggle_keyword(tb, FLG_IDENTIFIED); break;
	case EC_IK_STAR_IDENTIFIED: toggle_keyword(tb, FLG_STAR_IDENTIFIED); break;
	case EC_KK_WEAPONS: toggle_keyword(tb, FLG_WEAPONS); break;
	case EC_KK_FAVORITE_WEAPONS: toggle_keyword(tb, FLG_FAVORITE_WEAPONS); break;
	case EC_KK_ARMORS: toggle_keyword(tb, FLG_ARMORS); break;
	case EC_KK_MISSILES: toggle_keyword(tb, FLG_MISSILES); break;
	case EC_KK_DEVICES: toggle_keyword(tb, FLG_DEVICES); break;
	case EC_KK_LIGHTS: toggle_keyword(tb, FLG_LIGHTS); break;
	case EC_KK_JUNKS: toggle_keyword(tb, FLG_JUNKS); break;
	case EC_KK_CORPSES: toggle_keyword(tb, FLG_CORPSES); break;
	case EC_KK_SPELLBOOKS: toggle_keyword(tb, FLG_SPELLBOOKS); break;
	case EC_KK_SHIELDS: toggle_keyword(tb, FLG_SHIELDS); break;
	case EC_KK_BOWS: toggle_keyword(tb, FLG_BOWS); break;
	case EC_KK_RINGS: toggle_keyword(tb, FLG_RINGS); break;
	case EC_KK_AMULETS: toggle_keyword(tb, FLG_AMULETS); break;
	case EC_KK_SUITS: toggle_keyword(tb, FLG_SUITS); break;
	case EC_KK_CLOAKS: toggle_keyword(tb, FLG_CLOAKS); break;
	case EC_KK_HELMS: toggle_keyword(tb, FLG_HELMS); break;
	case EC_KK_GLOVES: toggle_keyword(tb, FLG_GLOVES); break;
	case EC_KK_BOOTS: toggle_keyword(tb, FLG_BOOTS); break;
	case EC_OK_COLLECTING: toggle_keyword(tb, FLG_COLLECTING); break;
	case EC_OK_BOOSTED: toggle_keyword(tb, FLG_BOOSTED); break;
	case EC_OK_MORE_DICE: toggle_keyword(tb, FLG_MORE_DICE); break;
	case EC_OK_MORE_BONUS: toggle_keyword(tb, FLG_MORE_BONUS); break;
	case EC_OK_WORTHLESS: toggle_keyword(tb, FLG_WORTHLESS); break;
	case EC_OK_ARTIFACT: toggle_keyword(tb, FLG_ARTIFACT); break;
	case EC_OK_EGO: toggle_keyword(tb, FLG_EGO); break;
	case EC_OK_GOOD: toggle_keyword(tb, FLG_GOOD); break;
	case EC_OK_NAMELESS: toggle_keyword(tb, FLG_NAMELESS); break;
	case EC_OK_AVERAGE: toggle_keyword(tb, FLG_AVERAGE); break;
	case EC_OK_RARE: toggle_keyword(tb, FLG_RARE); break;
	case EC_OK_COMMON: toggle_keyword(tb, FLG_COMMON); break;
	case EC_OK_WANTED: toggle_keyword(tb, FLG_WANTED); break;
	case EC_OK_UNIQUE: toggle_keyword(tb, FLG_UNIQUE); break;
	case EC_OK_HUMAN: toggle_keyword(tb, FLG_HUMAN); break;
	case EC_OK_UNREADABLE:
		toggle_keyword(tb, FLG_UNREADABLE);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_REALM1:
		toggle_keyword(tb, FLG_REALM1);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_REALM2:
		toggle_keyword(tb, FLG_REALM2);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_FIRST:
		toggle_keyword(tb, FLG_FIRST);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_SECOND:
		toggle_keyword(tb, FLG_SECOND);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_THIRD:
		toggle_keyword(tb, FLG_THIRD);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	case EC_OK_FOURTH:
		toggle_keyword(tb, FLG_FOURTH);
		add_keyword(tb, FLG_SPELLBOOKS);
		break;
	}

	tb->old_com_id = com_id;
	return FALSE;
}


/*
 * Check special key code and get a movement command id
 */
static int analyze_move_key(text_body_type *tb, int skey)
{
	int com_id;
	if (!(skey & SKEY_MASK)) return 0;

	switch (skey & ~SKEY_MOD_MASK)
	{
	case SKEY_DOWN:   com_id = EC_DOWN;   break;
	case SKEY_LEFT:   com_id = EC_LEFT;   break;
	case SKEY_RIGHT:  com_id = EC_RIGHT;  break;
	case SKEY_UP:     com_id = EC_UP;     break;
	case SKEY_PGUP:   com_id = EC_PGUP;   break;
	case SKEY_PGDOWN: com_id = EC_PGDOWN; break;
	case SKEY_TOP:    com_id = EC_TOP;    break;
	case SKEY_BOTTOM: com_id = EC_BOTTOM; break;
	default:
		return 0;
	}

	if (!(skey & SKEY_MOD_SHIFT))
	{
		/*
		 * Un-shifted cursor keys cancells
		 * selection created by shift+cursor.
		 */
		if (tb->mark & MARK_BY_SHIFT)
		{
			tb->mark = 0;
			tb->dirty_flags |= DIRTY_ALL;
		}

		return com_id;
	}

	if (tb->mark) return com_id;

	int len = strlen(tb->lines_list[tb->cy]);
	tb->mark = MARK_MARK | MARK_BY_SHIFT;
	tb->my = tb->cy;
	tb->mx = tb->cx;
	if (tb->cx > len) tb->mx = len;

	if (com_id == EC_UP || com_id == EC_DOWN)
	{
		tb->dirty_flags |= DIRTY_ALL;
	}
	else
	{
		tb->dirty_line = tb->cy;
	}

	return com_id;
}

/*
 * In-game editor of Object Auto-picker/Destoryer
 * @param player_ptr プレーヤーへの参照ポインタ
 */
void do_cmd_edit_autopick(player_type *player_ptr)
{
	static int cx_save = 0;
	static int cy_save = 0;
	autopick_type an_entry, *entry = &an_entry;
	char buf[MAX_LINELEN];
	int i;
	int key = -1;
	static s32b old_autosave_turn = 0L;
	byte quit = 0;

	text_body_type text_body;
	text_body_type *tb = &text_body;
	tb->changed = FALSE;
	tb->cx = cx_save;
	tb->cy = cy_save;
	tb->upper = tb->left = 0;
	tb->mark = 0;
	tb->mx = tb->my = 0;
	tb->old_cy = tb->old_upper = tb->old_left = -1;
	tb->old_wid = tb->old_hgt = -1;
	tb->old_com_id = 0;

	tb->yank = NULL;
	tb->search_o_ptr = NULL;
	tb->search_str = NULL;
	tb->last_destroyed = NULL;
	tb->dirty_flags = DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
	tb->dirty_line = -1;
	tb->filename_mode = PT_DEFAULT;

	if (current_world_ptr->game_turn < old_autosave_turn)
	{
		while (old_autosave_turn > current_world_ptr->game_turn) old_autosave_turn -= TURNS_PER_TICK * TOWN_DAWN;
	}

	if (current_world_ptr->game_turn > old_autosave_turn + 100L)
	{
		do_cmd_save_game(player_ptr, TRUE);
		old_autosave_turn = current_world_ptr->game_turn;
	}

	update_playtime();
	init_autopick();
	if (autopick_last_destroyed_object.k_idx)
	{
		autopick_entry_from_object(player_ptr, entry, &autopick_last_destroyed_object);
		tb->last_destroyed = autopick_line_from_entry_kill(entry);
	}

	tb->lines_list = read_pickpref_text_lines(player_ptr, &tb->filename_mode);
	for (i = 0; i < tb->cy; i++)
	{
		if (!tb->lines_list[i])
		{
			tb->cy = tb->cx = 0;
			break;
		}
	}

	screen_save();
	while (!quit)
	{
		int com_id = 0;
		draw_text_editor(player_ptr, tb);
		prt(_("(^Q:終了 ^W:セーブして終了, ESC:メニュー, その他:入力)",
			"(^Q:Quit, ^W:Save&Quit, ESC:Menu, Other:Input text)"), 0, 0);
		if (!tb->mark)
		{
			prt(format("(%d,%d)", tb->cx, tb->cy), 0, 60);
		}
		else
		{
			prt(format("(%d,%d)-(%d,%d)", tb->mx, tb->my, tb->cx, tb->cy), 0, 60);
		}

		Term_gotoxy(tb->cx - tb->left, tb->cy - tb->upper + 1);
		tb->dirty_flags = 0;
		tb->dirty_line = -1;
		tb->old_cy = tb->cy;
		tb->old_upper = tb->upper;
		tb->old_left = tb->left;
		tb->old_wid = tb->wid;
		tb->old_hgt = tb->hgt;

		key = inkey_special(TRUE);

		if (key & SKEY_MASK)
		{
			com_id = analyze_move_key(tb, key);
		}
		else if (key == ESCAPE)
		{
			com_id = do_command_menu(0, 0);
			tb->dirty_flags |= DIRTY_SCREEN;
		}
		else if (!iscntrl((unsigned char)key))
		{
			if (tb->mark)
			{
				tb->mark = 0;
				tb->dirty_flags |= DIRTY_ALL;
			}

			insert_single_letter(tb, key);
			continue;
		}
		else
		{
			com_id = get_com_id((char)key);
		}

		if (com_id) quit = do_editor_command(player_ptr, tb, com_id);
	}

	screen_load();
	strcpy(buf, pickpref_filename(player_ptr, tb->filename_mode));

	if (quit == QUIT_AND_SAVE)
		write_text_lines(buf, tb->lines_list);

	free_text_lines(tb->lines_list);
	string_free(tb->search_str);
	string_free(tb->last_destroyed);
	kill_yank_chain(tb);

	process_autopick_file(player_ptr, buf);
	current_world_ptr->start_time = (u32b)time(NULL);
	cx_save = tb->cx;
	cy_save = tb->cy;
}
