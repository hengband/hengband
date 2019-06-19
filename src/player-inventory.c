#include "angband.h"
#include "core.h"
#include "util.h"
#include "player-inventory.h"

#include "term.h"
#include "object.h"
#include "objectkind.h"
#include "object-flavor.h"
#include "floor.h"
#include "player-move.h"

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
void display_equip(OBJECT_TYPE_VALUE tval)
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
		if (select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr, tval))
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
	if (!item_tester_okay(&p_ptr->inventory_list[i], item_tester_tval)) return (FALSE);

	/* Assume okay */
	return (TRUE);
}

/*!
 * @brief 規定の処理にできるアイテムがプレイヤーの利用可能範囲内にあるかどうかを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(OBJECT_TYPE_VALUE tval)
{
	int j;
	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num = 0;

	for (j = 0; j < INVEN_TOTAL; j++)
		if (item_tester_okay(&p_ptr->inventory_list[j], tval))
			return TRUE;

	floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);
	if (floor_num)
		return TRUE;

	return FALSE;
}


/*!
 * @brief 床オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" p_ptr->inventory_list object with the given "tag".
 * @param cp 対応するタグIDを与える参照ポインタ
 * @param tag 該当するオブジェクトがあるかを調べたいタグ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 * @return タグに該当するオブジェクトがあるならTRUEを返す
 * @details
 * A "tag" is a numeral "n" appearing as "@n" anywhere in the\n
 * inscription of an object.  Alphabetical characters don't work as a\n
 * tag in this form.\n
 *\n
 * Also, the tag "@xn" will work as well, where "n" is a any tag-char,\n
 * and "x" is the "current" command_cmd code.\n
 */
static bool get_tag_floor(COMMAND_CODE *cp, char tag, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
	COMMAND_CODE i;
	concptr s;

	/**** Find a tag in the form of {@x#} (allow alphabet tag) ***/

	/* Check every object in the grid */
	for (i = 0; i < floor_num && i < 23; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the special tags */
			if ((s[1] == command_cmd) && (s[2] == tag))
			{
				/* Save the actual floor object ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}


	/**** Find a tag in the form of {@#} (allows only numerals)  ***/

	/* Don't allow {@#} with '#' being alphabet */
	if (tag < '0' || '9' < tag)
	{
		/* No such tag */
		return FALSE;
	}

	/* Check every object in the grid */
	for (i = 0; i < floor_num && i < 23; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the floor object ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}


/*!
 * @brief 所持/装備オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" p_ptr->inventory_list object with the given "tag".
 * @param cp 対応するタグIDを与える参照ポインタ
 * @param tag 該当するオブジェクトがあるかを調べたいタグ
 * @param mode 所持、装備の切り替え
 * @return タグに該当するオブジェクトがあるならTRUEを返す
 * @details
 * A "tag" is a numeral "n" appearing as "@n" anywhere in the\n
 * inscription of an object.  Alphabetical characters don't work as a\n
 * tag in this form.\n
 *\n
 * Also, the tag "@xn" will work as well, where "n" is a any tag-char,\n
 * and "x" is the "current" command_cmd code.\n
 */
static bool get_tag(COMMAND_CODE *cp, char tag, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
{
	COMMAND_CODE i;
	COMMAND_CODE start, end;
	concptr s;

	/* Extract index from mode */
	switch (mode)
	{
	case USE_EQUIP:
		start = INVEN_RARM;
		end = INVEN_TOTAL - 1;
		break;

	case USE_INVEN:
		start = 0;
		end = INVEN_PACK - 1;
		break;

	default:
		return FALSE;
	}

	/**** Find a tag in the form of {@x#} (allow alphabet tag) ***/

	/* Check every p_ptr->inventory_list object */
	for (i = start; i <= end; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Skip non-choice */
		if (!item_tester_okay(o_ptr, tval) && !(mode & USE_FULL)) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the special tags */
			if ((s[1] == command_cmd) && (s[2] == tag))
			{
				/* Save the actual p_ptr->inventory_list ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}


	/**** Find a tag in the form of {@#} (allows only numerals)  ***/

	/* Don't allow {@#} with '#' being alphabet */
	if (tag < '0' || '9' < tag)
	{
		/* No such tag */
		return FALSE;
	}

	/* Check every object */
	for (i = start; i <= end; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Skip non-choice */
		if (!item_tester_okay(o_ptr, tval) && !(mode & USE_FULL)) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the actual p_ptr->inventory_list ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す /
 * Move around label characters with correspond tags
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param mode 所持品リストか装備品リストかの切り替え
 * @return なし
 */
void prepare_label_string(char *label, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
{
	concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int  offset = (mode == USE_EQUIP) ? INVEN_RARM : 0;
	int  i;

	/* Prepare normal labels */
	strcpy(label, alphabet_chars);

	/* Move each label */
	for (i = 0; i < 52; i++)
	{
		COMMAND_CODE index;
		SYMBOL_CODE c = alphabet_chars[i];

		/* Find a tag with this label */
		if (get_tag(&index, c, mode, tval))
		{
			/* Delete the overwritten label */
			if (label[i] == c) label[i] = ' ';

			/* Move the label to the place of corresponding tag */
			label[index - offset] = c;
		}
	}
}


/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す(床上アイテム用) /
 * Move around label characters with correspond tags (floor version)
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 * @return なし
 */
 /*
  */
static void prepare_label_string_floor(char *label, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
	concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int  i;

	/* Prepare normal labels */
	strcpy(label, alphabet_chars);

	/* Move each label */
	for (i = 0; i < 52; i++)
	{
		COMMAND_CODE index;
		SYMBOL_CODE c = alphabet_chars[i];

		/* Find a tag with this label */
		if (get_tag_floor(&index, c, floor_list, floor_num))
		{
			/* Delete the overwritten label */
			if (label[i] == c) label[i] = ' ';

			/* Move the label to the place of corresponding tag */
			label[index] = c;
		}
	}
}

/*!
 * @brief 所持アイテムの表示を行う /
 * Display the p_ptr->inventory_list.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 * @details
 * Hack -- do not display "trailing" empty slots
 */
COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
{
	COMMAND_CODE i;
	int j, k, l, z = 0;
	int             col, cur_col, len;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char            tmp_val[80];
	COMMAND_CODE    out_index[23];
	TERM_COLOR      out_color[23];
	char            out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;
	TERM_LEN wid, hgt;
	char inven_label[52 + 1];

	/* Starting column */
	col = command_gap;

	Term_get_size(&wid, &hgt);

	/* Default "max-length" */
	len = wid - col - 1;


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Track */
		z = i + 1;
	}

	prepare_label_string(inven_label, USE_INVEN, tval);

	/* Display the p_ptr->inventory_list */
	for (k = 0, i = 0; i < z; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr, tval) && !(mode & USE_FULL)) continue;

		object_desc(o_name, o_ptr, 0);

		/* Save the object index, color, and description */
		out_index[k] = i;
		out_color[k] = tval_to_attr[o_ptr->tval % 128];

		/* Grey out charging items */
		if (o_ptr->timeout)
		{
			out_color[k] = TERM_L_DARK;
		}

		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		/* Account for icon if displayed */
		if (show_item_graph)
		{
			l += 2;
			if (use_bigtile) l++;
		}

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > wid - 4) ? 0 : (wid - len - 1);

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
		else if (i <= INVEN_PACK)
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", inven_label[i]);
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


		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, cur_col);

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			(void)sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
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
 * @brief 選択したアイテムの確認処理の補助 /
 * Verify the choice of an item.
 * @param prompt メッセージ表示の一部
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 */
static bool verify(concptr prompt, INVENTORY_IDX item)
{
	GAME_TEXT o_name[MAX_NLEN];
	char        out_val[MAX_NLEN + 20];
	object_type *o_ptr;


	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}

	/* Floor */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}
	object_desc(o_name, o_ptr, 0);

	/* Prompt */
	(void)sprintf(out_val, _("%s%sですか? ", "%s %s? "), prompt, o_name);

	/* Query */
	return (get_check(out_val));
}


/*!
 * @brief 選択したアイテムの確認処理のメインルーチン /
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 * Hack -- allow user to "prevent" certain choices
 */
static bool get_item_allow(INVENTORY_IDX item)
{
	concptr s;
	object_type *o_ptr;
	if (!command_cmd) return TRUE; /* command_cmd is no longer effective */

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}

	/* Floor */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}

	/* No inscription */
	if (!o_ptr->inscription) return (TRUE);

	/* Find a '!' */
	s = my_strchr(quark_str(o_ptr->inscription), '!');

	/* Process preventions */
	while (s)
	{
		/* Check the "restriction" */
		if ((s[1] == command_cmd) || (s[1] == '*'))
		{
			/* Verify the choice */
			if (!verify(_("本当に", "Really try"), item)) return (FALSE);
		}

		/* Find another '!' */
		s = my_strchr(s + 1, '!');
	}

	/* Allow it */
	return (TRUE);
}


/*!
 * @brief オブジェクト選択の汎用関数 /
 * Let the user select an item, save its "index"
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 * Return TRUE only if an acceptable item was chosen by the user.\n
 * @details
 * The selected item must satisfy the "item_tester_hook()" function,\n
 * if that hook is set, and the "item_tester_tval", if that value is set.\n
 *\n
 * All "item_tester" restrictions are cleared before this function returns.\n
 *\n
 * The user is allowed to choose acceptable items from the equipment,\n
 * p_ptr->inventory_list, or floor, respectively, if the proper flag was given,\n
 * and there are any acceptable items in that location.\n
 *\n
 * The equipment or p_ptr->inventory_list are displayed (even if no acceptable\n
 * items are in that location) if the proper flag was given.\n
 *\n
 * If there are no acceptable items available anywhere, and "str" is\n
 * not NULL, then it will be used as the text of a warning message\n
 * before the function returns.\n
 *\n
 * Note that the user must press "-" to specify the item on the floor,\n
 * and there is no way to "examine" the item on the floor, while the\n
 * use of "capital" letters will "examine" an p_ptr->inventory_list/equipment item,\n
 * and prompt for its use.\n
 *\n
 * If a legal item is selected from the p_ptr->inventory_list, we save it in "cp"\n
 * directly (0 to 35), and return TRUE.\n
 *\n
 * If a legal item is selected from the floor, we save it in "cp" as\n
 * a negative (-1 to -511), and return TRUE.\n
 *\n
 * If no item is available, we do nothing to "cp", and we display a\n
 * warning message, using "str" if available, and return FALSE.\n
 *\n
 * If no item is selected, we do nothing to "cp", and return FALSE.\n
 *\n
 * Global "p_ptr->command_new" is used when viewing the p_ptr->inventory_list or equipment\n
 * to allow the user to enter a command while viewing those screens, and\n
 * also to induce "auto-enter" of stores, and other such stuff.\n
 *\n
 * Global "p_ptr->command_see" may be set before calling this function to start\n
 * out in "browse" mode.  It is cleared before this function returns.\n
 *\n
 * Global "p_ptr->command_wrk" is used to choose between equip/inven listings.\n
 * If it is TRUE then we are viewing p_ptr->inventory_list, else equipment.\n
 *\n
 * We always erase the prompt when we are done, leaving a blank line,\n
 * or a warning message, if appropriate, if no items are available.\n
 */
bool get_item(OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	char which = ' ';

	int j;
	OBJECT_IDX k;
	OBJECT_IDX i1, i2;
	OBJECT_IDX e1, e2;

	bool done, item;

	bool oops = FALSE;

	bool equip = FALSE;
	bool inven = FALSE;
	bool floor = FALSE;

	bool allow_floor = FALSE;

	bool toggle = FALSE;

	char tmp_val[160];
	char out_val[160];

	int menu_line = (use_menu ? 1 : 0);
	int max_inven = 0;
	int max_equip = 0;

	static char prev_tag = '\0';
	char cur_tag = '\0';

	if (easy_floor || use_menu) return get_item_floor(cp, pmt, str, mode, tval);

	/* Extract args */
	if (mode & USE_EQUIP) equip = TRUE;
	if (mode & USE_INVEN) inven = TRUE;
	if (mode & USE_FLOOR) floor = TRUE;

	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* the_force */
		if (mode & USE_FORCE && (*cp == INVEN_FORCE))
		{
			tval = 0;
			item_tester_hook = NULL;
			command_cmd = 0; /* Hack -- command_cmd is no longer effective */
			return (TRUE);
		}

		/* Floor item? */
		else if (floor && (*cp < 0))
		{
			object_type *o_ptr;

			/* Special index */
			k = 0 - (*cp);
			o_ptr = &current_floor_ptr->o_list[k];

			/* Validate the item */
			if (item_tester_okay(o_ptr, tval) || (mode & USE_FULL))
			{
				/* Forget restrictions */
				tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}

		else if ((inven && (*cp >= 0) && (*cp < INVEN_PACK)) ||
			(equip && (*cp >= INVEN_RARM) && (*cp < INVEN_TOTAL)))
		{
			if (prev_tag && command_cmd)
			{
				/* Look up the tag and validate the item */
				if (!get_tag(&k, prev_tag, (*cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN, tval)) /* Reject */;
				else if ((k < INVEN_RARM) ? !inven : !equip) /* Reject */;
				else if (!get_item_okay(k)) /* Reject */;
				else
				{
					/* Accept that choice */
					(*cp) = k;

					/* Forget restrictions */
					tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Verify the item */
			else if (get_item_okay(*cp))
			{
				/* Forget restrictions */
				tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}
	}
	msg_print(NULL);

	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;


	/* Full p_ptr->inventory_list */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid p_ptr->inventory_list */
	if (!inven) i2 = -1;
	else if (use_menu)
	{
		for (j = 0; j < INVEN_PACK; j++)
			if (item_tester_okay(&p_ptr->inventory_list[j], tval) || (mode & USE_FULL)) max_inven++;
	}

	/* Restrict p_ptr->inventory_list indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;


	/* Full equipment */
	e1 = INVEN_RARM;
	e2 = INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!equip) e2 = -1;
	else if (use_menu)
	{
		for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
			if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(&p_ptr->inventory_list[j], tval) || (mode & USE_FULL)) max_equip++;
		if (p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT)) max_equip++;
	}

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	if (equip && p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
	{
		if (p_ptr->migite)
		{
			if (e2 < INVEN_LARM) e2 = INVEN_LARM;
		}
		else if (p_ptr->hidarite) e1 = INVEN_RARM;
	}


	/* Restrict floor usage */
	if (floor)
	{
		/* Scan all objects in the grid */
		for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			/* Accept the item on the floor if legal */
			if ((item_tester_okay(o_ptr, tval) || (mode & USE_FULL)) && (o_ptr->marked & OM_FOUND)) allow_floor = TRUE;
		}
	}

	/* Require at least one legal choice */
	if (!allow_floor && (i1 > i2) && (e1 > e2))
	{
		/* Cancel p_ptr->command_see */
		command_see = FALSE;
		oops = TRUE;
		done = TRUE;

		if (mode & USE_FORCE) {
			*cp = INVEN_FORCE;
			item = TRUE;
		}
	}

	/* Analyze choices */
	else
	{
		/* Hack -- Start on equipment if requested */
		if (command_see && command_wrk && equip)
		{
			command_wrk = TRUE;
		}

		/* Use p_ptr->inventory_list if allowed */
		else if (inven)
		{
			command_wrk = FALSE;
		}

		/* Use equipment if allowed */
		else if (equip)
		{
			command_wrk = TRUE;
		}

		/* Use p_ptr->inventory_list for floor */
		else
		{
			command_wrk = FALSE;
		}
	}


	/*
	 * 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する
	 */
	if ((always_show_list == TRUE) || use_menu) command_see = TRUE;

	/* Hack -- start out in "display" mode */
	if (command_see)
	{
		screen_save();
	}


	/* Repeat until done */
	while (!done)
	{
		COMMAND_CODE get_item_label = 0;

		/* Show choices */
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < 8; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (window_flag[j] & (PW_EQUIP)) ne++;
		}

		/* Toggle if needed */
		if ((command_wrk && ni && !ne) || (!command_wrk && !ni && ne))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		handle_stuff();

		/* Inventory screen */
		if (!command_wrk)
		{
			/* Redraw if needed */
			if (command_see) get_item_label = show_inven(menu_line, mode, tval);
		}

		/* Equipment screen */
		else
		{
			/* Redraw if needed */
			if (command_see) get_item_label = show_equip(menu_line, mode, tval);
		}

		/* Viewing p_ptr->inventory_list */
		if (!command_wrk)
		{
			/* Begin the prompt */
			sprintf(out_val, _("持ち物:", "Inven:"));

			/* Some legal items */
			if ((i1 <= i2) && !use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(i1), index_to_label(i2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (equip) strcat(out_val, format(_(" %s 装備品,", " %s for Equip,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "/")));
		}

		/* Viewing equipment */
		else
		{
			/* Begin the prompt */
			sprintf(out_val, _("装備品:", "Equip:"));

			/* Some legal items */
			if ((e1 <= e2) && !use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(e1), index_to_label(e2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (inven) strcat(out_val, format(_(" %s 持ち物,", " %s for Inven,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "'/'")));
		}

		/* Indicate legality of the "floor" item */
		if (allow_floor) strcat(out_val, _(" '-'床上,", " - for floor,"));
		if (mode & USE_FORCE) strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

		/* Finish the prompt */
		strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		which = inkey();

		if (use_menu)
		{
			int max_line = (command_wrk ? max_equip : max_inven);
			switch (which)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
			case '0':
			{
				done = TRUE;
				break;
			}

			case '8':
			case 'k':
			case 'K':
			{
				menu_line += (max_line - 1);
				break;
			}

			case '2':
			case 'j':
			case 'J':
			{
				menu_line++;
				break;
			}

			case '4':
			case '6':
			case 'h':
			case 'H':
			case 'l':
			case 'L':
			{
				/* Verify legality */
				if (!inven || !equip)
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				command_wrk = !command_wrk;
				max_line = (command_wrk ? max_equip : max_inven);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case 'x':
			case 'X':
			case '\r':
			case '\n':
			{
				if (command_wrk == USE_FLOOR)
				{
					/* Special index */
					(*cp) = -get_item_label;
				}
				else
				{
					/* Validate the item */
					if (!get_item_okay(get_item_label))
					{
						bell();
						break;
					}

					/* Allow player to "refuse" certain actions */
					if (!get_item_allow(get_item_label))
					{
						done = TRUE;
						break;
					}

					/* Accept that choice */
					(*cp) = get_item_label;
				}

				item = TRUE;
				done = TRUE;
				break;
			}
			case 'w':
			{
				if (mode & USE_FORCE) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}
			}
			}
			if (menu_line > max_line) menu_line -= max_line;
		}
		else
		{
			/* Parse it */
			switch (which)
			{
			case ESCAPE:
			{
				done = TRUE;
				break;
			}

			case '*':
			case '?':
			case ' ':
			{
				/* Hide the list */
				if (command_see)
				{
					/* Flip flag */
					command_see = FALSE;
					screen_load();
				}

				/* Show the list */
				else
				{
					screen_save();

					/* Flip flag */
					command_see = TRUE;
				}
				break;
			}

			case '/':
			{
				/* Verify legality */
				if (!inven || !equip)
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				command_wrk = !command_wrk;

				/* Need to redraw */
				break;
			}

			case '-':
			{
				/* Use floor item */
				if (allow_floor)
				{
					/* Scan all objects in the grid */
					for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
					{
						object_type *o_ptr;
						o_ptr = &current_floor_ptr->o_list[this_o_idx];
						next_o_idx = o_ptr->next_o_idx;

						/* Validate the item */
						if (!item_tester_okay(o_ptr, tval) && !(mode & USE_FULL)) continue;

						/* Special index */
						k = 0 - this_o_idx;

						/* Verify the item (if required) */
						if (other_query_flag && !verify(_("本当に", "Try"), k)) continue;

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k)) continue;

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						break;
					}

					/* Outer break */
					if (done) break;
				}

				bell();
				break;
			}

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				/* Look up the tag */
				if (!get_tag(&k, which, command_wrk ? USE_EQUIP : USE_INVEN, tval))
				{
					bell();
					break;
				}

				/* Hack -- Validate the item */
				if ((k < INVEN_RARM) ? !inven : !equip)
				{
					bell();
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				cur_tag = which;
				break;
			}

#if 0
			case '\n':
			case '\r':
			{
				/* Choose "default" p_ptr->inventory_list item */
				if (!command_wrk)
				{
					k = ((i1 == i2) ? i1 : -1);
				}

				/* Choose "default" equipment item */
				else
				{
					k = ((e1 == e2) ? e1 : -1);
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
#endif

			case 'w':
			{
				if (mode & USE_FORCE) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}

				/* Fall through */
			}

			default:
			{
				int ver;
				bool not_found = FALSE;

				/* Look up the alphabetical tag */
				if (!get_tag(&k, which, command_wrk ? USE_EQUIP : USE_INVEN, tval))
				{
					not_found = TRUE;
				}

				/* Hack -- Validate the item */
				else if ((k < INVEN_RARM) ? !inven : !equip)
				{
					not_found = TRUE;
				}

				/* Validate the item */
				else if (!get_item_okay(k))
				{
					not_found = TRUE;
				}

				if (!not_found)
				{
					/* Accept that choice */
					(*cp) = k;
					item = TRUE;
					done = TRUE;
					cur_tag = which;
					break;
				}

				/* Extract "query" setting */
				ver = isupper(which);
				which = (char)tolower(which);

				/* Convert letter to p_ptr->inventory_list index */
				if (!command_wrk)
				{
					if (which == '(') k = i1;
					else if (which == ')') k = i2;
					else k = label_to_inven(which);
				}

				/* Convert letter to equipment index */
				else
				{
					if (which == '(') k = e1;
					else if (which == ')') k = e2;
					else k = label_to_equip(which);
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Verify the item */
				if (ver && !verify(_("本当に", "Try"), k))
				{
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
			}
		}
	}


	/* Fix the screen if necessary */
	if (command_see)
	{
		screen_load();

		/* Hack -- Cancel "display" */
		command_see = FALSE;
	}


	/* Forget the tval restriction */
	tval = 0;

	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;


	/* Clean up  'show choices' */
	/* Toggle again if needed */
	if (toggle) toggle_inven_equip();

	p_ptr->window |= (PW_INVEN | PW_EQUIP);
	handle_stuff();

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str) msg_print(str);

	if (item)
	{
		repeat_push(*cp);
		if (command_cmd) prev_tag = cur_tag;
		command_cmd = 0; /* Hack -- command_cmd is no longer effective */
	}
	return (item);
}

/*
 * Choose an item and get auto-picker entry from it.
 */
object_type *choose_object(OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option, OBJECT_TYPE_VALUE tval)
{
	OBJECT_IDX item;
	if (!get_item(&item, q, s, option, tval)) return NULL;
	if (idx) *idx = item;

	if (item == INVEN_FORCE) return NULL;

	/* Get the item (in the pack) */
	else if (item >= 0) return &p_ptr->inventory_list[item];

	/* Get the item (on the floor) */
	else return &current_floor_ptr->o_list[0 - item];
}


/*!
 * @brief 床下に落ちているオブジェクトの数を返す / scan_floor
 * @param items オブジェクトのIDリストを返すための配列参照ポインタ
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param mode オプションフラグ
 * @return 対象のマスに落ちているアイテム数
 * @details
 * Return a list of o_list[] indexes of items at the given current_floor_ptr->grid_array
 * location. Valid flags are:
 *
 *		mode & 0x01 -- Item tester
 *		mode & 0x02 -- Marked items only
 *		mode & 0x04 -- Stop after first
 */
ITEM_NUMBER scan_floor(OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode)
{
	OBJECT_IDX this_o_idx, next_o_idx;

	ITEM_NUMBER num = 0;

	/* Sanity */
	if (!in_bounds(y, x)) return 0;

	/* Scan all objects in the grid */
	for (this_o_idx = current_floor_ptr->grid_array[y][x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Item tester */
		if ((mode & 0x01) && !item_tester_okay(o_ptr, item_tester_tval)) continue;

		/* Marked */
		if ((mode & 0x02) && !(o_ptr->marked & OM_FOUND)) continue;

		/* Accept this item */
		/* XXX Hack -- Enforce limit */
		if (num < 23)
			items[num] = this_o_idx;

		num++;

		/* Only one */
		if (mode & 0x04) break;
	}
	return num;
}


/*!
 * @brief 床下に落ちているアイテムの一覧を返す / Display a list of the items on the floor at the given location.
 * @param target_item カーソルの初期値
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param min_width 表示の長さ
 * @return 選択したアイテムの添え字
 * @details
 */
COMMAND_CODE show_floor(int target_item, POSITION y, POSITION x, TERM_LEN *min_width)
{
	COMMAND_CODE i, m;
	int j, k, l;
	int col, len;

	object_type *o_ptr;

	GAME_TEXT o_name[MAX_NLEN];
	char tmp_val[80];

	COMMAND_CODE out_index[23];
	TERM_COLOR out_color[23];
	char out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;

	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num;
	TERM_LEN wid, hgt;
	char floor_label[52 + 1];

	bool dont_need_to_show_weights = TRUE;

	Term_get_size(&wid, &hgt);

	/* Default length */
	len = MAX((*min_width), 20);

	/* Scan for objects in the grid, using item_tester_okay() */
	floor_num = scan_floor(floor_list, y, x, 0x03);

	/* Display the floor objects */
	for (k = 0, i = 0; i < floor_num && i < 23; i++)
	{
		o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		object_desc(o_name, o_ptr, 0);

		/* Save the index */
		out_index[k] = i;

		/* Acquire p_ptr->inventory_list color */
		out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];

		/* Save the object description */
		strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		if (o_ptr->tval != TV_GOLD) dont_need_to_show_weights = FALSE;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	if (show_weights && dont_need_to_show_weights) len -= 9;

	/* Save width */
	*min_width = len;

	/* Find the column to start in */
	col = (len > wid - 4) ? 0 : (wid - len - 1);

	prepare_label_string_floor(floor_label, floor_list, floor_num);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		m = floor_list[out_index[j]];
		o_ptr = &current_floor_ptr->o_list[m];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		if (use_menu && target_item)
		{
			if (j == (target_item - 1))
			{
				strcpy(tmp_val, _("》", "> "));
				target_item_label = m;
			}
			else strcpy(tmp_val, "   ");
		}
		else
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", floor_label[j]);
		}

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (show_weights && (o_ptr->tval != TV_GOLD))
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, j + 1, wid - 9);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	return target_item_label;
}

/*!
 * @brief オブジェクト選択の汎用関数(床上アイテム用) /
 * Let the user select an item, save its "index"
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 */
bool get_item_floor(COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
{
	char n1 = ' ', n2 = ' ', which = ' ';

	int j;
	COMMAND_CODE i1, i2;
	COMMAND_CODE e1, e2;
	COMMAND_CODE k;

	bool done, item;

	bool oops = FALSE;

	/* Extract args */
	bool equip = (mode & USE_EQUIP) ? TRUE : FALSE;
	bool inven = (mode & USE_INVEN) ? TRUE : FALSE;
	bool floor = (mode & USE_FLOOR) ? TRUE : FALSE;
	bool force = (mode & USE_FORCE) ? TRUE : FALSE;

	bool allow_equip = FALSE;
	bool allow_inven = FALSE;
	bool allow_floor = FALSE;

	bool toggle = FALSE;

	char tmp_val[160];
	char out_val[160];

	ITEM_NUMBER floor_num;
	OBJECT_IDX floor_list[23];
	int floor_top = 0;
	TERM_LEN min_width = 0;

	int menu_line = (use_menu ? 1 : 0);
	int max_inven = 0;
	int max_equip = 0;

	static char prev_tag = '\0';
	char cur_tag = '\0';

	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* the_force */
		if (force && (*cp == INVEN_FORCE))
		{
			tval = 0;
			item_tester_hook = NULL;
			command_cmd = 0; /* Hack -- command_cmd is no longer effective */
			return (TRUE);
		}

		/* Floor item? */
		else if (floor && (*cp < 0))
		{
			if (prev_tag && command_cmd)
			{
				/* Scan all objects in the grid */
				floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);

				/* Look up the tag */
				if (get_tag_floor(&k, prev_tag, floor_list, floor_num))
				{
					/* Accept that choice */
					(*cp) = 0 - floor_list[k];

					/* Forget restrictions */
					tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Validate the item */
			else if (item_tester_okay(&current_floor_ptr->o_list[0 - (*cp)], tval) || (mode & USE_FULL))
			{
				/* Forget restrictions */
				tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}

		else if ((inven && (*cp >= 0) && (*cp < INVEN_PACK)) ||
			(equip && (*cp >= INVEN_RARM) && (*cp < INVEN_TOTAL)))
		{
			if (prev_tag && command_cmd)
			{
				/* Look up the tag and validate the item */
				if (!get_tag(&k, prev_tag, (*cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN, tval)) /* Reject */;
				else if ((k < INVEN_RARM) ? !inven : !equip) /* Reject */;
				else if (!get_item_okay(k)) /* Reject */;
				else
				{
					/* Accept that choice */
					(*cp) = k;

					/* Forget restrictions */
					tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Verify the item */
			else if (get_item_okay(*cp))
			{
				/* Forget restrictions */
				tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}
	}

	msg_print(NULL);


	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;


	/* Full p_ptr->inventory_list */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid p_ptr->inventory_list */
	if (!inven) i2 = -1;
	else if (use_menu)
	{
		for (j = 0; j < INVEN_PACK; j++)
			if (item_tester_okay(&p_ptr->inventory_list[j], tval) || (mode & USE_FULL)) max_inven++;
	}

	/* Restrict p_ptr->inventory_list indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;


	/* Full equipment */
	e1 = INVEN_RARM;
	e2 = INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!equip) e2 = -1;
	else if (use_menu)
	{
		for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
			if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(&p_ptr->inventory_list[j], tval) || (mode & USE_FULL)) max_equip++;
		if (p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT)) max_equip++;
	}

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	if (equip && p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
	{
		if (p_ptr->migite)
		{
			if (e2 < INVEN_LARM) e2 = INVEN_LARM;
		}
		else if (p_ptr->hidarite) e1 = INVEN_RARM;
	}


	/* Count "okay" floor items */
	floor_num = 0;

	/* Restrict floor usage */
	if (floor)
	{
		/* Scan all objects in the grid */
		floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);
	}

	/* Accept p_ptr->inventory_list */
	if (i1 <= i2) allow_inven = TRUE;

	/* Accept equipment */
	if (e1 <= e2) allow_equip = TRUE;

	/* Accept floor */
	if (floor_num) allow_floor = TRUE;

	/* Require at least one legal choice */
	if (!allow_inven && !allow_equip && !allow_floor)
	{
		/* Cancel p_ptr->command_see */
		command_see = FALSE;
		oops = TRUE;
		done = TRUE;

		if (force) {
			*cp = INVEN_FORCE;
			item = TRUE;
		}
	}

	/* Analyze choices */
	else
	{
		/* Hack -- Start on equipment if requested */
		if (command_see && (command_wrk == (USE_EQUIP))
			&& allow_equip)
		{
			command_wrk = (USE_EQUIP);
		}

		/* Use p_ptr->inventory_list if allowed */
		else if (allow_inven)
		{
			command_wrk = (USE_INVEN);
		}

		/* Use equipment if allowed */
		else if (allow_equip)
		{
			command_wrk = (USE_EQUIP);
		}

		/* Use floor if allowed */
		else if (allow_floor)
		{
			command_wrk = (USE_FLOOR);
		}
	}

	/*
	 * 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する
	 */
	if ((always_show_list == TRUE) || use_menu) command_see = TRUE;

	/* Hack -- start out in "display" mode */
	if (command_see)
	{
		screen_save();
	}

	/* Repeat until done */
	while (!done)
	{
		COMMAND_CODE get_item_label = 0;

		/* Show choices */
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < 8; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (window_flag[j] & (PW_EQUIP)) ne++;
		}

		/* Toggle if needed */
		if ((command_wrk == (USE_EQUIP) && ni && !ne) ||
			(command_wrk == (USE_INVEN) && !ni && ne))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		handle_stuff();

		/* Inventory screen */
		if (command_wrk == (USE_INVEN))
		{
			/* Extract the legal requests */
			n1 = I2A(i1);
			n2 = I2A(i2);

			/* Redraw if needed */
			if (command_see) get_item_label = show_inven(menu_line, mode, tval);
		}

		/* Equipment screen */
		else if (command_wrk == (USE_EQUIP))
		{
			/* Extract the legal requests */
			n1 = I2A(e1 - INVEN_RARM);
			n2 = I2A(e2 - INVEN_RARM);

			/* Redraw if needed */
			if (command_see) get_item_label = show_equip(menu_line, mode, tval);
		}

		/* Floor screen */
		else if (command_wrk == (USE_FLOOR))
		{
			j = floor_top;
			k = MIN(floor_top + 23, floor_num) - 1;

			/* Extract the legal requests */
			n1 = I2A(j - floor_top);
			n2 = I2A(k - floor_top);

			/* Redraw if needed */
			if (command_see) get_item_label = show_floor(menu_line, p_ptr->y, p_ptr->x, &min_width);
		}

		/* Viewing p_ptr->inventory_list */
		if (command_wrk == (USE_INVEN))
		{
			/* Begin the prompt */
			sprintf(out_val, _("持ち物:", "Inven:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(i1), index_to_label(i2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (allow_equip)
			{
				if (!use_menu)
					strcat(out_val, _(" '/' 装備品,", " / for Equip,"));
				else if (allow_floor)
					strcat(out_val, _(" '6' 装備品,", " 6 for Equip,"));
				else
					strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
			}

			/* Append */
			if (allow_floor)
			{
				if (!use_menu)
					strcat(out_val, _(" '-'床上,", " - for floor,"));
				else if (allow_equip)
					strcat(out_val, _(" '4' 床上,", " 4 for floor,"));
				else
					strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
			}
		}

		/* Viewing equipment */
		else if (command_wrk == (USE_EQUIP))
		{
			/* Begin the prompt */
			sprintf(out_val, _("装備品:", "Equip:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(e1), index_to_label(e2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (allow_inven)
			{
				if (!use_menu)
					strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
				else if (allow_floor)
					strcat(out_val, _(" '4' 持ち物,", " 4 for Inven,"));
				else
					strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
			}

			/* Append */
			if (allow_floor)
			{
				if (!use_menu)
					strcat(out_val, _(" '-'床上,", " - for floor,"));
				else if (allow_inven)
					strcat(out_val, _(" '6' 床上,", " 6 for floor,"));
				else
					strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
			}
		}

		/* Viewing floor */
		else if (command_wrk == (USE_FLOOR))
		{
			/* Begin the prompt */
			sprintf(out_val, _("床上:", "Floor:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), n1, n2);

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			if (use_menu)
			{
				if (allow_inven && allow_equip)
				{
					strcat(out_val, _(" '4' 装備品, '6' 持ち物,", " 4 for Equip, 6 for Inven,"));
				}
				else if (allow_inven)
				{
					strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
				}
				else if (allow_equip)
				{
					strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
				}
			}
			/* Append */
			else if (allow_inven)
			{
				strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
			}
			else if (allow_equip)
			{
				strcat(out_val, _(" '/'装備品,", " / for Equip,"));
			}

			/* Append */
			if (command_see && !use_menu)
			{
				strcat(out_val, _(" Enter 次,", " Enter for scroll down,"));
			}
		}

		/* Append */
		if (force) strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

		/* Finish the prompt */
		strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		which = inkey();

		if (use_menu)
		{
			int max_line = 1;
			if (command_wrk == USE_INVEN) max_line = max_inven;
			else if (command_wrk == USE_EQUIP) max_line = max_equip;
			else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
			switch (which)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
			case '0':
			{
				done = TRUE;
				break;
			}

			case '8':
			case 'k':
			case 'K':
			{
				menu_line += (max_line - 1);
				break;
			}

			case '2':
			case 'j':
			case 'J':
			{
				menu_line++;
				break;
			}

			case '4':
			case 'h':
			case 'H':
			{
				/* Verify legality */
				if (command_wrk == (USE_INVEN))
				{
					if (allow_floor) command_wrk = USE_FLOOR;
					else if (allow_equip) command_wrk = USE_EQUIP;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (allow_inven) command_wrk = USE_INVEN;
					else if (allow_floor) command_wrk = USE_FLOOR;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_equip) command_wrk = USE_EQUIP;
					else if (allow_inven) command_wrk = USE_INVEN;
					else
					{
						bell();
						break;
					}
				}
				else
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				if (command_wrk == USE_INVEN) max_line = max_inven;
				else if (command_wrk == USE_EQUIP) max_line = max_equip;
				else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case '6':
			case 'l':
			case 'L':
			{
				/* Verify legality */
				if (command_wrk == (USE_INVEN))
				{
					if (allow_equip) command_wrk = USE_EQUIP;
					else if (allow_floor) command_wrk = USE_FLOOR;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (allow_floor) command_wrk = USE_FLOOR;
					else if (allow_inven) command_wrk = USE_INVEN;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_inven) command_wrk = USE_INVEN;
					else if (allow_equip) command_wrk = USE_EQUIP;
					else
					{
						bell();
						break;
					}
				}
				else
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				if (command_wrk == USE_INVEN) max_line = max_inven;
				else if (command_wrk == USE_EQUIP) max_line = max_equip;
				else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case 'x':
			case 'X':
			case '\r':
			case '\n':
			{
				if (command_wrk == USE_FLOOR)
				{
					/* Special index */
					(*cp) = -get_item_label;
				}
				else
				{
					/* Validate the item */
					if (!get_item_okay(get_item_label))
					{
						bell();
						break;
					}

					/* Allow player to "refuse" certain actions */
					if (!get_item_allow(get_item_label))
					{
						done = TRUE;
						break;
					}

					/* Accept that choice */
					(*cp) = get_item_label;
				}

				item = TRUE;
				done = TRUE;
				break;
			}
			case 'w':
			{
				if (force) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}
			}
			}
			if (menu_line > max_line) menu_line -= max_line;
		}
		else
		{
			/* Parse it */
			switch (which)
			{
			case ESCAPE:
			{
				done = TRUE;
				break;
			}

			case '*':
			case '?':
			case ' ':
			{
				/* Hide the list */
				if (command_see)
				{
					/* Flip flag */
					command_see = FALSE;
					screen_load();
				}

				/* Show the list */
				else
				{
					screen_save();

					/* Flip flag */
					command_see = TRUE;
				}
				break;
			}

			case '\n':
			case '\r':
			case '+':
			{
				int i;
				OBJECT_IDX o_idx;
				grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];

				if (command_wrk != (USE_FLOOR)) break;

				/* Get the object being moved. */
				o_idx = g_ptr->o_idx;

				/* Only rotate a pile of two or more objects. */
				if (!(o_idx && current_floor_ptr->o_list[o_idx].next_o_idx)) break;

				/* Remove the first object from the list. */
				excise_object_idx(o_idx);

				/* Find end of the list. */
				i = g_ptr->o_idx;
				while (current_floor_ptr->o_list[i].next_o_idx)
					i = current_floor_ptr->o_list[i].next_o_idx;

				/* Add after the last object. */
				current_floor_ptr->o_list[i].next_o_idx = o_idx;

				/* Re-scan floor list */
				floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				break;
			}

			case '/':
			{
				if (command_wrk == (USE_INVEN))
				{
					if (!allow_equip)
					{
						bell();
						break;
					}
					command_wrk = (USE_EQUIP);
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (!allow_inven)
					{
						bell();
						break;
					}
					command_wrk = (USE_INVEN);
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_inven)
					{
						command_wrk = (USE_INVEN);
					}
					else if (allow_equip)
					{
						command_wrk = (USE_EQUIP);
					}
					else
					{
						bell();
						break;
					}
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Need to redraw */
				break;
			}

			case '-':
			{
				if (!allow_floor)
				{
					bell();
					break;
				}

				/*
				 * If we are already examining the floor, and there
				 * is only one item, we will always select it.
				 * If we aren't examining the floor and there is only
				 * one item, we will select it if floor_query_flag
				 * is FALSE.
				 */
				if (floor_num == 1)
				{
					if ((command_wrk == (USE_FLOOR)) || (!carry_query_flag))
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;

						break;
					}
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				command_wrk = (USE_FLOOR);

				break;
			}

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				if (command_wrk != USE_FLOOR)
				{
					/* Look up the tag */
					if (!get_tag(&k, which, command_wrk, tval))
					{
						bell();
						break;
					}

					/* Hack -- Validate the item */
					if ((k < INVEN_RARM) ? !inven : !equip)
					{
						bell();
						break;
					}

					/* Validate the item */
					if (!get_item_okay(k))
					{
						bell();
						break;
					}
				}
				else
				{
					/* Look up the alphabetical tag */
					if (get_tag_floor(&k, which, floor_list, floor_num))
					{
						/* Special index */
						k = 0 - floor_list[k];
					}
					else
					{
						bell();
						break;
					}
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				cur_tag = which;
				break;
			}

#if 0
			case '\n':
			case '\r':
			{
				/* Choose "default" p_ptr->inventory_list item */
				if (command_wrk == (USE_INVEN))
				{
					k = ((i1 == i2) ? i1 : -1);
				}

				/* Choose "default" equipment item */
				else if (command_wrk == (USE_EQUIP))
				{
					k = ((e1 == e2) ? e1 : -1);
				}

				/* Choose "default" floor item */
				else if (command_wrk == (USE_FLOOR))
				{
					if (floor_num == 1)
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
					}
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
#endif

			case 'w':
			{
				if (force) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}

				/* Fall through */
			}

			default:
			{
				int ver;

				if (command_wrk != USE_FLOOR)
				{
					bool not_found = FALSE;

					/* Look up the alphabetical tag */
					if (!get_tag(&k, which, command_wrk, tval))
					{
						not_found = TRUE;
					}

					/* Hack -- Validate the item */
					else if ((k < INVEN_RARM) ? !inven : !equip)
					{
						not_found = TRUE;
					}

					/* Validate the item */
					else if (!get_item_okay(k))
					{
						not_found = TRUE;
					}

					if (!not_found)
					{
						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						cur_tag = which;
						break;
					}
				}
				else
				{
					/* Look up the alphabetical tag */
					if (get_tag_floor(&k, which, floor_list, floor_num))
					{
						/* Special index */
						k = 0 - floor_list[k];

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						cur_tag = which;
						break;
					}
				}

				/* Extract "query" setting */
				ver = isupper(which);
				which = (char)tolower(which);

				/* Convert letter to p_ptr->inventory_list index */
				if (command_wrk == (USE_INVEN))
				{
					if (which == '(') k = i1;
					else if (which == ')') k = i2;
					else k = label_to_inven(which);
				}

				/* Convert letter to equipment index */
				else if (command_wrk == (USE_EQUIP))
				{
					if (which == '(') k = e1;
					else if (which == ')') k = e2;
					else k = label_to_equip(which);
				}

				/* Convert letter to floor index */
				else if (command_wrk == USE_FLOOR)
				{
					if (which == '(') k = 0;
					else if (which == ')') k = floor_num - 1;
					else k = islower(which) ? A2I(which) : -1;
					if (k < 0 || k >= floor_num || k >= 23)
					{
						bell();
						break;
					}

					/* Special index */
					k = 0 - floor_list[k];
				}

				/* Validate the item */
				if ((command_wrk != USE_FLOOR) && !get_item_okay(k))
				{
					bell();
					break;
				}

				/* Verify the item */
				if (ver && !verify(_("本当に", "Try"), k))
				{
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
			}
		}
	}

	/* Fix the screen if necessary */
	if (command_see)
	{
		screen_load();

		/* Hack -- Cancel "display" */
		command_see = FALSE;
	}


	/* Forget the tval restriction */
	tval = 0;

	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;


	/* Clean up  'show choices' */
	/* Toggle again if needed */
	if (toggle) toggle_inven_equip();

	p_ptr->window |= (PW_INVEN | PW_EQUIP);
	handle_stuff();

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str) msg_print(str);

	if (item)
	{
		repeat_push(*cp);
		if (command_cmd) prev_tag = cur_tag;
		command_cmd = 0; /* Hack -- command_cmd is no longer effective */
	}
	return (item);
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(void)
{
	OBJECT_IDX this_o_idx;
	concptr q, s;
	OBJECT_IDX item;

	/* Restrict the choices */
	item_tester_hook = inven_carry_okay;

	/* Get an object */
	q = _("どれを拾いますか？", "Get which item? ");
	s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");

	if (choose_object(&item, q, s, (USE_FLOOR), 0))
	{
		this_o_idx = 0 - item;
	}
	else
	{
		return (FALSE);
	}

	/* Pick up the object */
	py_pickup_aux(this_o_idx);

	return (TRUE);
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @return なし
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(bool pickup)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	GAME_TEXT o_name[MAX_NLEN];
	object_type *o_ptr;

	int floor_num = 0;
	OBJECT_IDX floor_o_idx = 0;

	int can_pickup = 0;

	/* Scan the pile of objects */
	for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		/* Access the object */
		o_ptr = &current_floor_ptr->o_list[this_o_idx];

		object_desc(o_name, o_ptr, 0);

		/* Access the next object */
		next_o_idx = o_ptr->next_o_idx;

		disturb(FALSE, FALSE);

		/* Pick up gold */
		if (o_ptr->tval == TV_GOLD)
		{
#ifdef JP
			msg_format(" $%ld の価値がある%sを見つけた。",
				(long)o_ptr->pval, o_name);
#else
			msg_format("You have found %ld gold pieces worth of %s.",
				(long)o_ptr->pval, o_name);
#endif

			/* Collect the gold */
			p_ptr->au += o_ptr->pval;

			/* Redraw gold */
			p_ptr->redraw |= (PR_GOLD);

			p_ptr->window |= (PW_PLAYER);

			/* Delete the gold */
			delete_object_idx(this_o_idx);

			/* Check the next object */
			continue;
		}
		else if (o_ptr->marked & OM_NOMSG)
		{
			/* If 0 or 1 non-NOMSG items are in the pile, the NOMSG ones are
			 * ignored. Otherwise, they are included in the prompt. */
			o_ptr->marked &= ~(OM_NOMSG);
			continue;
		}

		/* Count non-gold objects that can be picked up. */
		if (inven_carry_okay(o_ptr))
		{
			can_pickup++;
		}

		/* Count non-gold objects */
		floor_num++;

		/* Remember this index */
		floor_o_idx = this_o_idx;
	}

	/* There are no non-gold objects */
	if (!floor_num)
		return;

	/* Mention the number of objects */
	if (!pickup)
	{
		/* One object */
		if (floor_num == 1)
		{
			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void)sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			msg_format(_("%sがある。", "You see %s."), o_name);
		}

		/* Multiple objects */
		else
		{
			msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);
		}

		return;
	}

	/* The player has no room for anything on the floor. */
	if (!can_pickup)
	{
		/* One object */
		if (floor_num == 1)
		{
			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void)sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
		}

		/* Multiple objects */
		else
		{
			msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

		}

		return;
	}

	/* One object */
	if (floor_num == 1)
	{
		/* Hack -- query every object */
		if (carry_query_flag)
		{
			char out_val[MAX_NLEN + 20];

			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void)sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			/* Build a prompt */
			(void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);

			/* Ask the user to confirm */
			if (!get_check(out_val))
			{
				return;
			}
		}

		/* Access the object */
		o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

		/* Option: Make object sensing easy */
		if (easy_sense)
		{
			/* Sense the object */
			(void)sense_object(o_ptr);
		}

#endif /* ALLOW_EASY_SENSE */

		/* Pick up the object */
		py_pickup_aux(floor_o_idx);
	}

	/* Allow the user to choose an object */
	else
	{
		while (can_pickup--)
		{
			if (!py_pickup_floor_aux()) break;
		}
	}
}



/*!
 * @brief 所持アイテム一覧を表示する /
 * Choice window "shadow" of the "show_inven()" function
 * @return なし
 */
void display_inven(OBJECT_TYPE_VALUE tval)
{
	register int i, n, z = 0;
	object_type *o_ptr;
	TERM_COLOR attr = TERM_WHITE;
	char tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	TERM_LEN wid, hgt;

	Term_get_size(&wid, &hgt);

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;
		z = i + 1;
	}

	for (i = 0; i < z; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
		if (item_tester_okay(o_ptr, tval))
		{
			tmp_val[0] = index_to_label(i);
			tmp_val[1] = ')';
		}

		Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
		object_desc(o_name, o_ptr, 0);
		n = strlen(o_name);
		attr = tval_to_attr[o_ptr->tval % 128];
		if (o_ptr->timeout)
		{
			attr = TERM_L_DARK;
		}

		Term_putstr(3, i, n, attr, o_name);
		Term_erase(3 + n, i, 255);

		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif
			prt(tmp_val, i, wid - 9);
		}
	}

	for (i = z; i < hgt; i++)
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
COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval)
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
		if (!(select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr, tval) || (mode & USE_FULL)) &&
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

	prepare_label_string(equip_label, USE_EQUIP, tval);

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
 * @brief 所持/装備オブジェクトIDの部位表現を返す /
 * Return a string mentioning how a given item is carried
 * @param i 部位表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 部位表現の文字列ポインタ
 */
concptr mention_use(int i)
{
	concptr p;

	/* Examine the location */
	switch (i)
	{
#ifdef JP
	case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "運搬中" : ((p_ptr->ryoute && p_ptr->migite) ? " 両手" : (left_hander ? " 左手" : " 右手")); break;
#else
	case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "Just lifting" : (p_ptr->migite ? "Wielding" : "On arm"); break;
#endif

#ifdef JP
	case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "運搬中" : ((p_ptr->ryoute && p_ptr->hidarite) ? " 両手" : (left_hander ? " 右手" : " 左手")); break;
#else
	case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "Just lifting" : (p_ptr->hidarite ? "Wielding" : "On arm"); break;
#endif

	case INVEN_BOW:   p = (adj_str_hold[p_ptr->stat_ind[A_STR]] < p_ptr->inventory_list[i].weight / 10) ? _("運搬中", "Just holding") : _("射撃用", "Shooting"); break;
	case INVEN_RIGHT: p = (left_hander ? _("左手指", "On left hand") : _("右手指", "On right hand")); break;
	case INVEN_LEFT:  p = (left_hander ? _("右手指", "On right hand") : _("左手指", "On left hand")); break;
	case INVEN_NECK:  p = _("  首", "Around neck"); break;
	case INVEN_LITE:  p = _(" 光源", "Light source"); break;
	case INVEN_BODY:  p = _("  体", "On body"); break;
	case INVEN_OUTER: p = _("体の上", "About body"); break;
	case INVEN_HEAD:  p = _("  頭", "On head"); break;
	case INVEN_HANDS: p = _("  手", "On hands"); break;
	case INVEN_FEET:  p = _("  足", "On feet"); break;
	default:          p = _("ザック", "In pack"); break;
	}

	/* Return the result */
	return p;
}


/*!
 * @brief 所持/装備オブジェクトIDの現在の扱い方の状態表現を返す /
 * Return a string describing how a given item is being worn.
 * @param i 状態表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 状態表現内容の文字列ポインタ
 * @details
 * Currently, only used for items in the equipment, not p_ptr->inventory_list.
 */
concptr describe_use(int i)
{
	concptr p;

	switch (i)
	{
#ifdef JP
	case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "運搬中の" : ((p_ptr->ryoute && p_ptr->migite) ? "両手に装備している" : (left_hander ? "左手に装備している" : "右手に装備している")); break;
#else
	case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "just lifting" : (p_ptr->migite ? "attacking monsters with" : "wearing on your arm"); break;
#endif

#ifdef JP
	case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "運搬中の" : ((p_ptr->ryoute && p_ptr->hidarite) ? "両手に装備している" : (left_hander ? "右手に装備している" : "左手に装備している")); break;
#else
	case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "just lifting" : (p_ptr->hidarite ? "attacking monsters with" : "wearing on your arm"); break;
#endif

	case INVEN_BOW:   p = (adj_str_hold[p_ptr->stat_ind[A_STR]] < p_ptr->inventory_list[i].weight / 10) ? _("持つだけで精一杯の", "just holding") : _("射撃用に装備している", "shooting missiles with"); break;
	case INVEN_RIGHT: p = (left_hander ? _("左手の指にはめている", "wearing on your left hand") : _("右手の指にはめている", "wearing on your right hand")); break;
	case INVEN_LEFT:  p = (left_hander ? _("右手の指にはめている", "wearing on your right hand") : _("左手の指にはめている", "wearing on your left hand")); break;
	case INVEN_NECK:  p = _("首にかけている", "wearing around your neck"); break;
	case INVEN_LITE:  p = _("光源にしている", "using to light the way"); break;
	case INVEN_BODY:  p = _("体に着ている", "wearing on your body"); break;
	case INVEN_OUTER: p = _("身にまとっている", "wearing on your back"); break;
	case INVEN_HEAD:  p = _("頭にかぶっている", "wearing on your head"); break;
	case INVEN_HANDS: p = _("手につけている", "wearing on your hands"); break;
	case INVEN_FEET:  p = _("足にはいている", "wearing on your feet"); break;
	default:          p = _("ザックに入っている", "carrying in your pack"); break;
	}

	/* Return the result */
	return p;
}

