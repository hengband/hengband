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
#include "autopick/autopick-matcher.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-keys-table.h"
#include "autopick/autopick-destroyer.h"
#include "autopick/autopick-describer.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-adder.h"
#include "autopick/autopick-pref-processor.h"
#include "gameterm.h"
#include "autopick/autopick.h"
#include "core.h"
#include "core/show-file.h"
#include "cmd/cmd-save.h"
#include "io/read-pref-file.h"

#include "mind.h"

#include "market/store.h"
#include "player-status.h"
#include "player-move.h"
#include "player-class.h"
#include "player-race.h"
#include "player-inventory.h"
#include "view/display-player.h" // 暫定。後で消す.
#include "object/object-kind.h"
#include "object-ego.h"
#include "object-flavor.h"
#include "object-hook.h"

#include "floor.h"
#include "world.h"
#include "monster.h"
#include "monsterrace.h"
#include "view/display-main-window.h" // 暫定。後で消す.

/*
 * Auto inscription
 */
static void auto_inscribe_item(player_type *player_ptr, object_type *o_ptr, int idx)
{
	if (idx < 0 || !autopick_list[idx].insc) return;

	if (!o_ptr->inscription)
		o_ptr->inscription = quark_add(autopick_list[idx].insc);

	player_ptr->window |= (PW_EQUIP | PW_INVEN);
	player_ptr->update |= (PU_BONUS);
}


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


static const char autoregister_header[] = "?:$AUTOREGISTER";

/*
 *  Clear auto registered lines in the picktype.prf .
 */
static bool clear_auto_register(player_type *player_ptr)
{
	char pref_file[1024];
	path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_WITH_PNAME));
	FILE *pref_fff;
	pref_fff = my_fopen(pref_file, "r");

	if (!pref_fff)
	{
		path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_DEFAULT));
		pref_fff = my_fopen(pref_file, "r");
	}

	if (!pref_fff)
	{
		return TRUE;
	}

	char tmp_file[1024];
	FILE *tmp_fff;
	tmp_fff = my_fopen_temp(tmp_file, sizeof(tmp_file));
	if (!tmp_fff)
	{
		fclose(pref_fff);
		msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), tmp_file);
		msg_print(NULL);
		return FALSE;
	}

	bool autoregister = FALSE;
	int num = 0;
	char buf[1024];
	while (TRUE)
	{
		if (my_fgets(pref_fff, buf, sizeof(buf))) break;

		if (autoregister)
		{
			if (buf[0] != '#' && buf[0] != '?') num++;
			continue;
		}

		if (streq(buf, autoregister_header))
		{
			autoregister = TRUE;
		}
		else
		{
			fprintf(tmp_fff, "%s\n", buf);
		}
	}

	my_fclose(pref_fff);
	my_fclose(tmp_fff);

	bool okay = TRUE;
	if (num)
	{
		msg_format(_("以前のキャラクター用の自動設定(%d行)が残っています。",
			"Auto registered lines (%d lines) for previous character are remaining."), num);
		strcpy(buf, _("古い設定行は削除します。よろしいですか？", "These lines will be deleted.  Are you sure? "));

		if (!get_check(buf))
		{
			okay = FALSE;
			autoregister = FALSE;

			msg_print(_("エディタのカット&ペースト等を使って必要な行を避難してください。",
				"Use cut & paste of auto picker editor (_) to keep old prefs."));
		}
	}

	if (autoregister)
	{
		tmp_fff = my_fopen(tmp_file, "r");
		pref_fff = my_fopen(pref_file, "w");

		while (!my_fgets(tmp_fff, buf, sizeof(buf)))
			fprintf(pref_fff, "%s\n", buf);

		my_fclose(pref_fff);
		my_fclose(tmp_fff);
	}

	fd_kill(tmp_file);
	return okay;
}


/*
 *  Automatically register an auto-destroy preference line
 */
bool autopick_autoregister(player_type *player_ptr, object_type *o_ptr)
{
	char buf[1024];
	char pref_file[1024];
	FILE *pref_fff;
	autopick_type an_entry, *entry = &an_entry;
	int match_autopick = find_autopick_list(player_ptr, o_ptr);
	if (match_autopick != -1)
	{
		concptr what;
		byte act = autopick_list[match_autopick].action;
		if (act & DO_AUTOPICK) what = _("自動で拾う", "auto-pickup");
		else if (act & DO_AUTODESTROY) what = _("自動破壊する", "auto-destroy");
		else if (act & DONT_AUTOPICK) what = _("放置する", "leave on floor");
		else what = _("確認して拾う", "query auto-pickup");

		msg_format(_("そのアイテムは既に%sように設定されています。", "The object is already registered to %s."), what);
		return FALSE;
	}

	if ((object_is_known(o_ptr) && object_is_artifact(o_ptr)) ||
		((o_ptr->ident & IDENT_SENSE) &&
		(o_ptr->feeling == FEEL_TERRIBLE || o_ptr->feeling == FEEL_SPECIAL)))
	{
		GAME_TEXT o_name[MAX_NLEN];
		object_desc(player_ptr, o_name, o_ptr, 0);
		msg_format(_("%sは破壊不能だ。", "You cannot auto-destroy %s."), o_name);
		return FALSE;
	}

	if (!player_ptr->autopick_autoregister)
	{
		if (!clear_auto_register(player_ptr)) return FALSE;
	}

	path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_WITH_PNAME));
	pref_fff = my_fopen(pref_file, "r");

	if (!pref_fff)
	{
		path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_DEFAULT));
		pref_fff = my_fopen(pref_file, "r");
	}

	if (pref_fff)
	{
		while (TRUE)
		{
			if (my_fgets(pref_fff, buf, sizeof(buf)))
			{
				player_ptr->autopick_autoregister = FALSE;
				break;
			}

			if (streq(buf, autoregister_header))
			{
				player_ptr->autopick_autoregister = TRUE;
				break;
			}
		}

		fclose(pref_fff);
	}
	else
	{
		/*
		 * File could not be opened for reading.  Assume header not
		 * present.
		 */
		player_ptr->autopick_autoregister = FALSE;
	}

	pref_fff = my_fopen(pref_file, "a");
	if (!pref_fff)
	{
		msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), pref_file);
		msg_print(NULL);
		return FALSE;
	}

	if (!player_ptr->autopick_autoregister)
	{
		fprintf(pref_fff, "%s\n", autoregister_header);

		fprintf(pref_fff, "%s\n", _("# *警告!!* 以降の行は自動登録されたものです。",
			"# *Warning!* The lines below will be deleted later."));
		fprintf(pref_fff, "%s\n", _("# 後で自動的に削除されますので、必要な行は上の方へ移動しておいてください。",
			"# Keep it by cut & paste if you need these lines for future characters."));
		player_ptr->autopick_autoregister = TRUE;
	}

	autopick_entry_from_object(player_ptr, entry, o_ptr);
	entry->action = DO_AUTODESTROY;
	add_autopick_list(entry);

	concptr tmp = autopick_line_from_entry(entry);
	fprintf(pref_fff, "%s\n", tmp);
	string_free(tmp);
	fclose(pref_fff);
	return TRUE;
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
 * Check if this line is expression or not.
 * And update it if it is.
 */
static void check_expression_line(text_body_type *tb, int y)
{
	concptr s = tb->lines_list[y];

	if ((s[0] == '?' && s[1] == ':') ||
		(tb->states[y] & LSTAT_BYPASS))
	{
		tb->dirty_flags |= DIRTY_EXPRESSION;
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
 * Insert return code and split the line
 */
static bool insert_return_code(text_body_type *tb)
{
	char buf[MAX_LINELEN];
	int i, j, num_lines;

	for (num_lines = 0; tb->lines_list[num_lines]; num_lines++);

	if (num_lines >= MAX_LINES - 2) return FALSE;
	num_lines--;

	for (; tb->cy < num_lines; num_lines--)
	{
		tb->lines_list[num_lines + 1] = tb->lines_list[num_lines];
		tb->states[num_lines + 1] = tb->states[num_lines];
	}

	for (i = j = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
	{
#ifdef JP
		if (iskanji(tb->lines_list[tb->cy][i]))
			buf[j++] = tb->lines_list[tb->cy][i++];
#endif
		buf[j++] = tb->lines_list[tb->cy][i];
	}

	buf[j] = '\0';
	tb->lines_list[tb->cy + 1] = string_make(&tb->lines_list[tb->cy][i]);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(buf);
	tb->dirty_flags |= DIRTY_EXPRESSION;
	tb->changed = TRUE;
	return TRUE;
}


/*
 * Choose an item for search
 */
static bool get_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
	concptr q = _("どのアイテムを検索しますか? ", "Enter which item? ");
	concptr s = _("アイテムを持っていない。", "You have nothing to enter.");
	object_type *o_ptr;
	o_ptr = choose_object(player_ptr, NULL, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP, 0);
	if (!o_ptr) return FALSE;

	*o_handle = o_ptr;
	string_free(*search_strp);
	char buf[MAX_NLEN + 20];
	object_desc(player_ptr, buf, *o_handle, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
	*search_strp = string_make(format("<%s>", buf));
	return TRUE;
}


/*
 * Prepare for search by destroyed object
 */
static bool get_destroyed_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
	if (!autopick_last_destroyed_object.k_idx) return FALSE;

	*o_handle = &autopick_last_destroyed_object;
	string_free(*search_strp);
	char buf[MAX_NLEN + 20];
	object_desc(player_ptr, buf, *o_handle, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
	*search_strp = string_make(format("<%s>", buf));
	return TRUE;
}


/*
 * Choose an item or string for search
 */
static byte get_string_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
	/*
	 * Text color
	 * TERM_YELLOW : Overwrite mode
	 * TERM_WHITE : Insert mode
	 */
	byte color = TERM_YELLOW;
	char buf[MAX_NLEN + 20];
	const int len = 80;
	char prompt[] = _("検索(^I:持ち物 ^L:破壊された物): ", "Search key(^I:inven ^L:destroyed): ");
	int col = sizeof(prompt) - 1;
	if (*search_strp) strcpy(buf, *search_strp);
	else buf[0] = '\0';

	if (*o_handle) color = TERM_L_GREEN;

	prt(prompt, 0, 0);
	int pos = 0;
	while (TRUE)
	{
		bool back = FALSE;
		Term_erase(col, 0, 255);
		Term_putstr(col, 0, -1, color, buf);
		Term_gotoxy(col + pos, 0);

		int skey = inkey_special(TRUE);
		switch (skey)
		{
		case SKEY_LEFT:
		case KTRL('b'):
		{
			int i = 0;
			color = TERM_WHITE;
			if (pos == 0) break;

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
			return 0;

		case KTRL('r'):
			back = TRUE;
			/* Fall through */

		case '\n':
		case '\r':
		case KTRL('s'):
			if (*o_handle) return (back ? -1 : 1);
			string_free(*search_strp);
			*search_strp = string_make(buf);
			*o_handle = NULL;
			return (back ? -1 : 1);

		case KTRL('i'):
			return get_object_for_search(player_ptr, o_handle, search_strp);

		case KTRL('l'):
			if (get_destroyed_object_for_search(player_ptr, o_handle, search_strp))
				return 1;
			break;

		case '\010':
		{
			int i = 0;
			color = TERM_WHITE;
			if (pos == 0) break;

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
			int dst, src;
			color = TERM_WHITE;
			if (buf[pos] == '\0') break;

			src = pos + 1;
#ifdef JP
			if (iskanji(buf[pos])) src++;
#endif
			dst = pos;
			while ('\0' != (buf[dst++] = buf[src++]));

			break;
		}

		default:
		{
			char tmp[100];
			char c;
			if (skey & SKEY_MASK) break;

			c = (char)skey;
			if (color != TERM_WHITE)
			{
				if (color == TERM_L_GREEN)
				{
					*o_handle = NULL;
					string_free(*search_strp);
					*search_strp = NULL;
				}

				buf[0] = '\0';
				color = TERM_WHITE;
			}

			strcpy(tmp, buf + pos);
#ifdef JP
			if (iskanji(c))
			{
				char next;
				inkey_base = TRUE;
				next = inkey();

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
			my_strcat(buf, tmp, len + 1);

			break;
		}
		}

		if (*o_handle == NULL || color == TERM_L_GREEN) continue;

		*o_handle = NULL;
		buf[0] = '\0';
		string_free(*search_strp);
		*search_strp = NULL;
	}
}


/*
 * Search next line matches for o_ptr
 */
static void search_for_object(player_type *player_ptr, text_body_type *tb, object_type *o_ptr, bool forward)
{
	autopick_type an_entry, *entry = &an_entry;
	GAME_TEXT o_name[MAX_NLEN];
	int bypassed_cy = -1;
	int i = tb->cy;
	object_desc(player_ptr, o_name, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
	str_tolower(o_name);

	while (TRUE)
	{
		bool match;
		if (forward)
		{
			if (!tb->lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}

		if (!autopick_new_entry(entry, tb->lines_list[i], FALSE)) continue;

		match = is_autopick_match(player_ptr, o_ptr, entry, o_name);
		autopick_free_entry(entry);
		if (!match)	continue;

		if (tb->states[i] & LSTAT_BYPASS)
		{
			if (bypassed_cy == -1) bypassed_cy = i;
			continue;
		}

		tb->cx = 0;
		tb->cy = i;
		if (bypassed_cy != -1)
		{
			tb->dirty_flags |= DIRTY_SKIP_INACTIVE;
		}

		return;
	}

	if (bypassed_cy == -1)
	{
		tb->dirty_flags |= DIRTY_NOT_FOUND;
		return;
	}

	tb->cx = 0;
	tb->cy = bypassed_cy;
	tb->dirty_flags |= DIRTY_INACTIVE;
}


/*
 * Search next line matches to the string
 */
static void search_for_string(text_body_type *tb, concptr search_str, bool forward)
{
	int bypassed_cy = -1;
	int bypassed_cx = 0;

	int i = tb->cy;
	while (TRUE)
	{
		concptr pos;
		if (forward)
		{
			if (!tb->lines_list[++i]) break;
		}
		else
		{
			if (--i < 0) break;
		}

		pos = my_strstr(tb->lines_list[i], search_str);
		if (!pos) continue;

		if ((tb->states[i] & LSTAT_BYPASS) &&
			!(tb->states[i] & LSTAT_EXPRESSION))
		{
			if (bypassed_cy == -1)
			{
				bypassed_cy = i;
				bypassed_cx = (int)(pos - tb->lines_list[i]);
			}

			continue;
		}

		tb->cx = (int)(pos - tb->lines_list[i]);
		tb->cy = i;

		if (bypassed_cy != -1)
		{
			tb->dirty_flags |= DIRTY_SKIP_INACTIVE;
		}

		return;
	}

	if (bypassed_cy == -1)
	{
		tb->dirty_flags |= DIRTY_NOT_FOUND;
		return;
	}

	tb->cx = bypassed_cx;
	tb->cy = bypassed_cy;
	tb->dirty_flags |= DIRTY_INACTIVE;
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
 * Draw text
 */
static void draw_text_editor(player_type *player_ptr, text_body_type *tb)
{
	int i;
	int by1 = 0, by2 = 0;

	Term_get_size(&tb->wid, &tb->hgt);

	/*
	 * Top line (-1), description line (-3), separator (-1)
	 *  == -5
	 */
	tb->hgt -= 2 + DESCRIPT_HGT;

#ifdef JP
	/* Don't let cursor at second byte of kanji */
	for (i = 0; tb->lines_list[tb->cy][i]; i++)
		if (iskanji(tb->lines_list[tb->cy][i]))
		{
			i++;
			if (i == tb->cx)
			{
				/*
				 * Move to a correct position in the
				 * left or right
				 */
				if (i & 1) tb->cx--;
				else tb->cx++;
				break;
			}
		}
#endif
	if (tb->cy < tb->upper || tb->upper + tb->hgt <= tb->cy)
		tb->upper = tb->cy - (tb->hgt) / 2;
	if (tb->upper < 0)
		tb->upper = 0;
	if ((tb->cx < tb->left + 10 && tb->left > 0) || tb->left + tb->wid - 5 <= tb->cx)
		tb->left = tb->cx - (tb->wid) * 2 / 3;
	if (tb->left < 0)
		tb->left = 0;

	if (tb->old_wid != tb->wid || tb->old_hgt != tb->hgt)
		tb->dirty_flags |= DIRTY_SCREEN;
	else if (tb->old_upper != tb->upper || tb->old_left != tb->left)
		tb->dirty_flags |= DIRTY_ALL;

	if (tb->dirty_flags & DIRTY_SCREEN)
	{
		tb->dirty_flags |= (DIRTY_ALL | DIRTY_MODE);
		Term_clear();
	}

	if (tb->dirty_flags & DIRTY_MODE)
	{
		char buf[MAX_LINELEN];
		int sepa_length = tb->wid;
		for (i = 0; i < sepa_length; i++)
			buf[i] = '-';
		buf[i] = '\0';
		Term_putstr(0, tb->hgt + 1, sepa_length, TERM_WHITE, buf);
	}

	if (tb->dirty_flags & DIRTY_EXPRESSION)
	{
		byte state = 0;
		for (int y = 0; tb->lines_list[y]; y++)
		{
			char f;
			concptr v;
			concptr s = tb->lines_list[y];
			char *ss, *s_keep;
			int s_len;

			tb->states[y] = state;

			if (*s++ != '?') continue;
			if (*s++ != ':') continue;

			if (streq(s, "$AUTOREGISTER"))
				state |= LSTAT_AUTOREGISTER;

			s_len = strlen(s);
			ss = (char *)string_make(s);
			s_keep = ss;

			v = process_pref_file_expr(player_ptr, &ss, &f);

			if (streq(v, "0")) state |= LSTAT_BYPASS;
			else state &= ~LSTAT_BYPASS;

			C_KILL(s_keep, s_len + 1, char);

			tb->states[y] = state | LSTAT_EXPRESSION;
		}

		tb->dirty_flags |= DIRTY_ALL;
	}

	if (tb->mark)
	{
		tb->dirty_flags |= DIRTY_ALL;

		by1 = MIN(tb->my, tb->cy);
		by2 = MAX(tb->my, tb->cy);
	}

	for (i = 0; i < tb->hgt; i++)
	{
		int j;
		int leftcol = 0;
		concptr msg;
		byte color;
		int y = tb->upper + i;

		if (!(tb->dirty_flags & DIRTY_ALL) && (tb->dirty_line != y))
			continue;

		msg = tb->lines_list[y];
		if (!msg) break;

		for (j = 0; *msg; msg++, j++)
		{
			if (j == tb->left) break;
#ifdef JP
			if (j > tb->left)
			{
				leftcol = 1;
				break;
			}
			if (iskanji(*msg))
			{
				msg++;
				j++;
			}
#endif
		}

		Term_erase(0, i + 1, tb->wid);
		if (tb->states[y] & LSTAT_AUTOREGISTER)
		{
			color = TERM_L_RED;
		}
		else
		{
			if (tb->states[y] & LSTAT_BYPASS) color = TERM_SLATE;
			else color = TERM_WHITE;
		}

		if (!tb->mark || (y < by1 || by2 < y))
		{
			Term_putstr(leftcol, i + 1, tb->wid - 1, color, msg);
		}
		else if (by1 != by2)
		{
			Term_putstr(leftcol, i + 1, tb->wid - 1, TERM_YELLOW, msg);
		}
		else
		{
			int x0 = leftcol + tb->left;
			int len = strlen(tb->lines_list[tb->cy]);
			int bx1 = MIN(tb->mx, tb->cx);
			int bx2 = MAX(tb->mx, tb->cx);

			if (bx2 > len) bx2 = len;

			Term_gotoxy(leftcol, i + 1);
			if (x0 < bx1) Term_addstr(bx1 - x0, color, msg);
			if (x0 < bx2) Term_addstr(bx2 - bx1, TERM_YELLOW, msg + (bx1 - x0));
			Term_addstr(-1, color, msg + (bx2 - x0));
		}
	}

	for (; i < tb->hgt; i++)
	{
		Term_erase(0, i + 1, tb->wid);
	}

	bool is_dirty_diary = (tb->dirty_flags & (DIRTY_ALL | DIRTY_NOT_FOUND | DIRTY_NO_SEARCH)) != 0;
	bool is_updated = tb->old_cy != tb->cy || is_dirty_diary || tb->dirty_line == tb->cy;
	if (is_updated) return;

	autopick_type an_entry, *entry = &an_entry;
	concptr str1 = NULL, str2 = NULL;
	for (i = 0; i < DESCRIPT_HGT; i++)
	{
		Term_erase(0, tb->hgt + 2 + i, tb->wid);
	}

	if (tb->dirty_flags & DIRTY_NOT_FOUND)
	{
		str1 = format(_("パターンが見つかりません: %s", "Pattern not found: %s"), tb->search_str);
	}
	else if (tb->dirty_flags & DIRTY_SKIP_INACTIVE)
	{
		str1 = format(_("無効状態の行をスキップしました。(%sを検索中)",
			"Some inactive lines are skipped. (Searching %s)"), tb->search_str);
	}
	else if (tb->dirty_flags & DIRTY_INACTIVE)
	{
		str1 = format(_("無効状態の行だけが見付かりました。(%sを検索中)",
			"Found only an inactive line. (Searching %s)"), tb->search_str);
	}
	else if (tb->dirty_flags & DIRTY_NO_SEARCH)
	{
		str1 = _("検索するパターンがありません(^S で検索)。", "No pattern to search. (Press ^S to search.)");
	}
	else if (tb->lines_list[tb->cy][0] == '#')
	{
		str1 = _("この行はコメントです。", "This line is a comment.");
	}
	else if (tb->lines_list[tb->cy][0] && tb->lines_list[tb->cy][1] == ':')
	{
		switch (tb->lines_list[tb->cy][0])
		{
		case '?':
			str1 = _("この行は条件分岐式です。", "This line is a Conditional Expression.");
			break;
		case 'A':
			str1 = _("この行はマクロの実行内容を定義します。", "This line defines a Macro action.");
			break;
		case 'P':
			str1 = _("この行はマクロのトリガー・キーを定義します。", "This line defines a Macro trigger key.");
			break;
		case 'C':
			str1 = _("この行はキー配置を定義します。", "This line defines a Keymap.");
			break;
		}

		switch (tb->lines_list[tb->cy][0])
		{
		case '?':
			if (tb->states[tb->cy] & LSTAT_BYPASS)
			{
				str2 = _("現在の式の値は「偽(=0)」です。", "The expression is 'False'(=0) currently.");
			}
			else
			{
				str2 = _("現在の式の値は「真(=1)」です。", "The expression is 'True'(=1) currently.");
			}
			break;

		default:
			if (tb->states[tb->cy] & LSTAT_AUTOREGISTER)
			{
				str2 = _("この行は後で削除されます。", "This line will be delete later.");
			}

			else if (tb->states[tb->cy] & LSTAT_BYPASS)
			{
				str2 = _("この行は現在は無効な状態です。", "This line is bypassed currently.");
			}
			break;
		}
	}
	else if (autopick_new_entry(entry, tb->lines_list[tb->cy], FALSE))
	{
		char buf[MAX_LINELEN];
		char temp[MAX_LINELEN];
		concptr t;

		describe_autopick(buf, entry);

		if (tb->states[tb->cy] & LSTAT_AUTOREGISTER)
		{
			strcat(buf, _("この行は後で削除されます。", "  This line will be delete later."));
		}

		if (tb->states[tb->cy] & LSTAT_BYPASS)
		{
			strcat(buf, _("この行は現在は無効な状態です。", "  This line is bypassed currently."));
		}

		roff_to_buf(buf, 81, temp, sizeof(temp));
		t = temp;
		for (i = 0; i < 3; i++)
		{
			if (t[0] == 0)
				break;
			else
			{
				prt(t, tb->hgt + 1 + 1 + i, 0);
				t += strlen(t) + 1;
			}
		}
		autopick_free_entry(entry);
	}

	if (str1) prt(str1, tb->hgt + 1 + 1, 0);
	if (str2) prt(str2, tb->hgt + 1 + 2, 0);
}


/*
 * Kill segment of a line
 */
static void kill_line_segment(text_body_type *tb, int y, int x0, int x1, bool whole)
{
	concptr s = tb->lines_list[y];
	if (whole && x0 == 0 && s[x1] == '\0' && tb->lines_list[y + 1])
	{
		string_free(tb->lines_list[y]);

		int i;
		for (i = y; tb->lines_list[i + 1]; i++)
			tb->lines_list[i] = tb->lines_list[i + 1];
		tb->lines_list[i] = NULL;

		tb->dirty_flags |= DIRTY_EXPRESSION;

		return;
	}

	if (x0 == x1) return;

	char buf[MAX_LINELEN];
	char *d = buf;
	for (int x = 0; x < x0; x++)
		*(d++) = s[x];

	for (int x = x1; s[x]; x++)
		*(d++) = s[x];

	*d = '\0';
	string_free(tb->lines_list[y]);
	tb->lines_list[y] = string_make(buf);
	check_expression_line(tb, y);
	tb->changed = TRUE;
}


/*
 * Get a trigger key and insert ASCII string for the trigger
 */
static bool insert_macro_line(text_body_type *tb)
{
	int i, n = 0;
	flush();
	inkey_base = TRUE;
	i = inkey();
	char buf[1024];
	while (i)
	{
		buf[n++] = (char)i;
		inkey_base = TRUE;
		inkey_scan = TRUE;
		i = inkey();
	}

	buf[n] = '\0';
	flush();

	char tmp[1024];
	ascii_to_text(tmp, buf);
	if (!tmp[0]) return FALSE;

	tb->cx = 0;
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("P:%s", tmp));

	i = macro_find_exact(buf);
	if (i == -1)
	{
		tmp[0] = '\0';
	}
	else
	{
		ascii_to_text(tmp, macro__act[i]);
	}

	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("A:%s", tmp));

	return TRUE;
}


/*
 * Get a command key and insert ASCII string for the key
 */
static bool insert_keymap_line(text_body_type *tb)
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

	flush();
	char buf[2];
	buf[0] = inkey();
	buf[1] = '\0';

	flush();
	char tmp[1024];
	ascii_to_text(tmp, buf);
	if (!tmp[0]) return FALSE;

	tb->cx = 0;
	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("C:%d:%s", mode, tmp));

	concptr act = keymap_act[mode][(byte)(buf[0])];
	if (act)
	{
		ascii_to_text(tmp, act);
	}

	insert_return_code(tb);
	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(format("A:%s", tmp));

	return TRUE;
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
 * Insert single letter at cursor position.
 */
static void insert_single_letter(text_body_type *tb, int key)
{
	int i, j, len;
	char buf[MAX_LINELEN];

	for (i = j = 0; tb->lines_list[tb->cy][i] && i < tb->cx; i++)
	{
		buf[j++] = tb->lines_list[tb->cy][i];
	}

#ifdef JP
	if (iskanji(key))
	{
		int next;

		inkey_base = TRUE;
		next = inkey();
		if (j + 2 < MAX_LINELEN)
		{
			buf[j++] = (char)key;
			buf[j++] = (char)next;
			tb->cx += 2;
		}
		else
			bell();
	}
	else
#endif
	{
		if (j + 1 < MAX_LINELEN)
			buf[j++] = (char)key;
		tb->cx++;
	}

	for (; tb->lines_list[tb->cy][i] && j + 1 < MAX_LINELEN; i++)
		buf[j++] = tb->lines_list[tb->cy][i];
	buf[j] = '\0';

	string_free(tb->lines_list[tb->cy]);
	tb->lines_list[tb->cy] = string_make(buf);
	len = strlen(tb->lines_list[tb->cy]);
	if (len < tb->cx) tb->cx = len;

	tb->dirty_line = tb->cy;
	check_expression_line(tb, tb->cy);
	tb->changed = TRUE;
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
	text_body_type text_body, *tb = &text_body;
	autopick_type an_entry, *entry = &an_entry;
	char buf[MAX_LINELEN];
	int i;
	int key = -1;
	static s32b old_autosave_turn = 0L;
	byte quit = 0;

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
