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
#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-editor-command.h"
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
