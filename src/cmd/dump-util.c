#include "angband.h"
#include "feature.h"
#include "dump-util.h"

TERM_COLOR attr_idx = 0;
SYMBOL_CODE char_idx = 0;

TERM_COLOR attr_idx_feat[F_LIT_MAX];
SYMBOL_CODE char_idx_feat[F_LIT_MAX];

/*
 * @brief シンボル変更処理 / Do visual mode command -- Change symbols
 * @param ch
 * @param visual_list_ptr
 * @param height
 * @param width
 * @param attr_ptr_ptr
 * @param char_left_ptr
 * @param cur_attr_ptr
 * @param cur_char_ptr
 * @param need_redraw
 * @return 何かコマンドを入れたらTRUE
 */
bool visual_mode_command(char ch, bool *visual_list_ptr,
	int height, int width,
	TERM_COLOR *attr_top_ptr, byte *char_left_ptr,
	TERM_COLOR *cur_attr_ptr, SYMBOL_CODE *cur_char_ptr, bool *need_redraw)
{
	static TERM_COLOR attr_old = 0;
	static SYMBOL_CODE char_old = 0;

	switch (ch)
	{
	case ESCAPE:
	{
		if (!*visual_list_ptr) return FALSE;

		*cur_attr_ptr = attr_old;
		*cur_char_ptr = char_old;
		*visual_list_ptr = FALSE;
		return TRUE;
	}
	case '\n':
	case '\r':
	{
		if (!*visual_list_ptr) return FALSE;

		*visual_list_ptr = FALSE;
		*need_redraw = TRUE;
		return TRUE;
	}
	case 'V':
	case 'v':
	{
		if (*visual_list_ptr) return FALSE;

		*visual_list_ptr = TRUE;
		*attr_top_ptr = MAX(0, (*cur_attr_ptr & 0x7f) - 5);
		*char_left_ptr = MAX(0, *cur_char_ptr - 10);
		attr_old = *cur_attr_ptr;
		char_old = *cur_char_ptr;
		return TRUE;
	}
	case 'C':
	case 'c':
	{
		attr_idx = *cur_attr_ptr;
		char_idx = *cur_char_ptr;
		for (int i = 0; i < F_LIT_MAX; i++)
		{
			attr_idx_feat[i] = 0;
			char_idx_feat[i] = 0;
		}

		return TRUE;
	}
	case 'P':
	case 'p':
	{
		if (attr_idx || (!(char_idx & 0x80) && char_idx))
		{
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
	}
	default:
	{
		if (!*visual_list_ptr) return FALSE;

		int eff_width;
		int d = get_keymap_dir(ch);
		TERM_COLOR a = (*cur_attr_ptr & 0x7f);
		SYMBOL_CODE c = *cur_char_ptr;

		if (use_bigtile) eff_width = width / 2;
		else eff_width = width;

		if ((a == 0) && (ddy[d] < 0)) d = 0;
		if ((c == 0) && (ddx[d] < 0)) d = 0;
		if ((a == 0x7f) && (ddy[d] > 0)) d = 0;
		if (((byte)c == 0xff) && (ddx[d] > 0)) d = 0;

		a += (TERM_COLOR)ddy[d];
		c += (SYMBOL_CODE)ddx[d];
		if (c & 0x80) a |= 0x80;

		*cur_attr_ptr = a;
		*cur_char_ptr = c;
		if ((ddx[d] < 0) && *char_left_ptr > MAX(0, (int)c - 10)) (*char_left_ptr)--;
		if ((ddx[d] > 0) && *char_left_ptr + eff_width < MIN(0xff, (int)c + 10)) (*char_left_ptr)++;
		if ((ddy[d] < 0) && *attr_top_ptr > MAX(0, (int)(a & 0x7f) - 4)) (*attr_top_ptr)--;
		if ((ddy[d] > 0) && *attr_top_ptr + height < MIN(0x7f, (a & 0x7f) + 4)) (*attr_top_ptr)++;

		return TRUE;
	}
	}

	return FALSE;
}
