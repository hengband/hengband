#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-key-flag-process.h"

/*
 * Delete or insert string
 */
void toggle_keyword(text_body_type *tb, BIT_FLAGS flg)
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
void toggle_command_letter(text_body_type *tb, byte flg)
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
void add_keyword(text_body_type *tb, BIT_FLAGS flg)
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
bool add_empty_line(text_body_type *tb)
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


static chain_str_type *new_chain_str(concptr str)
{
	chain_str_type *chain;
	size_t len = strlen(str);
	chain = (chain_str_type *)ralloc(sizeof(chain_str_type) + len * sizeof(char));
	strcpy(chain->s, str);
	chain->next = NULL;
	return chain;
}


void kill_yank_chain(text_body_type *tb)
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


void add_str_to_yank(text_body_type *tb, concptr str)
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
void copy_text_to_yank(text_body_type *tb)
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
