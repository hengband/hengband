﻿#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-dirty-flags.h"
#include "cmd-io/macro-util.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "term/screen-processor.h"
#include "util/string-processor.h"

/*
 * Check if this line is expression or not.
 * And update it if it is.
 */
void check_expression_line(text_body_type *tb, int y)
{
	concptr s = tb->lines_list[y];

	if ((s[0] == '?' && s[1] == ':') ||
		(tb->states[y] & LSTAT_BYPASS))
	{
		tb->dirty_flags |= DIRTY_EXPRESSION;
	}
}

/*
 * Insert return code and split the line
 */
bool insert_return_code(text_body_type *tb)
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
 * Get a trigger key and insert ASCII string for the trigger
 */
bool insert_macro_line(text_body_type *tb)
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
bool insert_keymap_line(text_body_type *tb)
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
 * Insert single letter at cursor position.
 */
void insert_single_letter(text_body_type *tb, int key)
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
 * Kill segment of a line
 */
void kill_line_segment(text_body_type *tb, int y, int x0, int x1, bool whole)
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
