#include "angband.h"
#include "core.h"
#include "util.h"
#include "term.h"
#include "object.h"
#include "objectkind.h"
#include "object-flavor.h"

#include "view-mainwindow.h"

bool select_ring_slot;

/*!
 * @brief プレイヤーの所持/装備オブジェクトIDが指輪枠かを返す /
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 指輪枠ならばTRUEを返す。
 */
bool is_ring_slot(int i)
{
	return (i == INVEN_RIGHT) || (i == INVEN_LEFT);
}


/*!
 * @brief 選択アルファベットラベルからプレイヤーの装備オブジェクトIDを返す /
 * Convert a label into the index of a item in the "equip"
 * @return 対応するID。該当スロットにオブジェクトが存在しなかった場合-1を返す / Return "-1" if the label does not indicate a real item
 */
INVENTORY_IDX label_to_equip(int c)
{
	INVENTORY_IDX i;

	/* Convert */
	i = (INVENTORY_IDX)(islower(c) ? A2I(c) : -1) + INVEN_RARM;

	/* Verify the index */
	if ((i < INVEN_RARM) || (i >= INVEN_TOTAL)) return (-1);

	if (select_ring_slot) return is_ring_slot(i) ? i : -1;

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory_list[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}




/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 * @return なし
 */
void display_equip(void)
{
	register int i, n;
	object_type *o_ptr;
	TERM_COLOR attr = TERM_WHITE;
	char tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	TERM_LEN wid, hgt;

	Term_get_size(&wid, &hgt);

	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
		if (select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr))
		{
			tmp_val[0] = index_to_label(i);
			tmp_val[1] = ')';
		}

		Term_putstr(0, i - INVEN_RARM, 3, TERM_WHITE, tmp_val);
		if ((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute)
		{
			strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
			attr = TERM_WHITE;
		}
		else
		{
			object_desc(o_name, o_ptr, 0);
			attr = tval_to_attr[o_ptr->tval % 128];
		}

		n = strlen(o_name);
		if (o_ptr->timeout)
		{
			attr = TERM_L_DARK;
		}
		Term_putstr(3, i - INVEN_RARM, n, attr, o_name);

		Term_erase(3 + n, i - INVEN_RARM, 255);

		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, i - INVEN_RARM, wid - (show_labels ? 28 : 9));
		}

		if (show_labels)
		{
			Term_putstr(wid - 20, i - INVEN_RARM, -1, TERM_WHITE, " <-- ");
			prt(mention_use(i), i - INVEN_RARM, wid - 15);
		}
	}

	for (i = INVEN_TOTAL - INVEN_RARM; i < hgt; i++)
	{
		Term_erase(0, i, 255);
	}
}


/*!
 * @brief 装備アイテムの表示を行う /
 * Display the equipment.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 */
COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode)
{
	COMMAND_CODE i;
	int j, k, l;
	int             col, cur_col, len;
	object_type *o_ptr;
	char            tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	COMMAND_CODE    out_index[23];
	TERM_COLOR      out_color[23];
	char            out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;
	TERM_LEN wid, hgt;
	char            equip_label[52 + 1];

	/* Starting column */
	col = command_gap;

	Term_get_size(&wid, &hgt);

	/* Maximal length */
	len = wid - col - 1;


	/* Scan the equipment list */
	for (k = 0, i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Is this item acceptable? */
		if (!(select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr) || (mode & USE_FULL)) &&
			(!((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute) ||
			(mode & IGNORE_BOTHHAND_SLOT))) continue;

		object_desc(o_name, o_ptr, 0);

		if ((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute)
		{
			(void)strcpy(out_desc[k], _("(武器を両手持ち)", "(wielding with two-hands)"));
			out_color[k] = TERM_WHITE;
		}
		else
		{
			(void)strcpy(out_desc[k], o_name);
			out_color[k] = tval_to_attr[o_ptr->tval % 128];
		}

		out_index[k] = i;
		/* Grey out charging items */
		if (o_ptr->timeout)
		{
			out_color[k] = TERM_L_DARK;
		}

		/* Extract the maximal length (see below) */
#ifdef JP
		l = strlen(out_desc[k]) + (2 + 1);
#else
		l = strlen(out_desc[k]) + (2 + 3);
#endif


		/* Increase length for labels (if needed) */
#ifdef JP
		if (show_labels) l += (7 + 2);
#else
		if (show_labels) l += (14 + 2);
#endif


		/* Increase length for weight (if needed) */
		if (show_weights) l += 9;

		if (show_item_graph) l += 2;

		/* Maintain the max-length */
		if (l > len) len = l;

		/* Advance the entry */
		k++;
	}

	/* Hack -- Find a column to start in */
#ifdef JP
	col = (len > wid - 6) ? 0 : (wid - len - 1);
#else
	col = (len > wid - 4) ? 0 : (wid - len - 1);
#endif

	prepare_label_string(equip_label, USE_EQUIP);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		i = out_index[j];
		o_ptr = &p_ptr->inventory_list[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		if (use_menu && target_item)
		{
			if (j == (target_item - 1))
			{
				strcpy(tmp_val, _("》", "> "));
				target_item_label = i;
			}
			else strcpy(tmp_val, "  ");
		}
		else if (i >= INVEN_RARM)
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", equip_label[i - INVEN_RARM]);
		}
		else
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", index_to_label(i));
		}

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		cur_col = col + 3;

		/* Display graphics for object, if desired */
		if (show_item_graph)
		{
			TERM_COLOR a = object_attr(o_ptr);
			SYMBOL_CODE c = object_char(o_ptr);
			Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
			if (use_bigtile) cur_col++;

			cur_col += 2;
		}

		/* Use labels */
		if (show_labels)
		{
			/* Mention the use */
			(void)sprintf(tmp_val, _("%-7s: ", "%-14s: "), mention_use(i));

			put_str(tmp_val, j + 1, cur_col);

			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j + 1, _(cur_col + 9, cur_col + 16));
		}

		/* No labels */
		else
		{
			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
		}

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			(void)sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			(void)sprintf(tmp_val, "%3d.%d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, j + 1, wid - 9);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Save the new column */
	command_gap = col;

	return target_item_label;
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 * @return なし
 */
void toggle_inven_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Flip inven to equip */
		if (window_flag[j] & (PW_INVEN))
		{
			/* Flip flags */
			window_flag[j] &= ~(PW_INVEN);
			window_flag[j] |= (PW_EQUIP);

			p_ptr->window |= (PW_EQUIP);
		}

		/* Flip inven to equip */
		else if (window_flag[j] & (PW_EQUIP))
		{
			/* Flip flags */
			window_flag[j] &= ~(PW_EQUIP);
			window_flag[j] |= (PW_INVEN);

			p_ptr->window |= (PW_INVEN);
		}
	}
}


/*!
 * @brief プレイヤーの所持/装備オブジェクトが正規のものかを返す /
 * Auxiliary function for "get_item()" -- test an index
 * @param i 選択アイテムID
 * @return 正規のIDならばTRUEを返す。
 */
bool get_item_okay(OBJECT_IDX i)
{
	/* Illegal items */
	if ((i < 0) || (i >= INVEN_TOTAL)) return (FALSE);

	if (select_ring_slot) return is_ring_slot(i);

	/* Verify the item */
	if (!item_tester_okay(&p_ptr->inventory_list[i])) return (FALSE);

	/* Assume okay */
	return (TRUE);
}

/*!
 * @brief プレイヤーがオブジェクトを拾うことができる状態かを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(void)
{
	int j;
	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num = 0;

	for (j = 0; j < INVEN_TOTAL; j++)
		if (item_tester_okay(&p_ptr->inventory_list[j]))
			return TRUE;

	floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);
	if (floor_num)
		return TRUE;

	return FALSE;
}
