﻿#include "view/display-inventory.h"
#include "flavor/flavor-describer.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "system/object-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

/*!
 * @brief 所持アイテムの表示を行う /
 * Display the inventory.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 * @details
 * Hack -- do not display "trailing" empty slots
 */
COMMAND_CODE show_inventory(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval)
{
    COMMAND_CODE i;
    int k, l, z = 0;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char tmp_val[80];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    char out_desc[23][MAX_NLEN];
    COMMAND_CODE target_item_label = 0;
    char inven_label[52 + 1];

    int col = command_gap;
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);
    int len = wid - col - 1;
    for (i = 0; i < INVEN_PACK; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        z = i + 1;
    }

    prepare_label_string(owner_ptr, inven_label, USE_INVEN, tval);
    for (k = 0, i = 0; i < z; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!item_tester_okay(owner_ptr, o_ptr, tval) && !(mode & USE_FULL))
            continue;

        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        out_index[k] = i;
        out_color[k] = tval_to_attr[o_ptr->tval % 128];
        if (o_ptr->timeout)
            out_color[k] = TERM_L_DARK;

        (void)strcpy(out_desc[k], o_name);
        l = strlen(out_desc[k]) + 5;
        if (show_weights)
            l += 9;

        if (show_item_graph) {
            l += 2;
            if (use_bigtile)
                l++;
        }

        if (l > len)
            len = l;

        k++;
    }

    col = (len > wid - 4) ? 0 : (wid - len - 1);
    int cur_col;
    int j;
    for (j = 0; j < k; j++) {
        i = out_index[j];
        o_ptr = &owner_ptr->inventory_list[i];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                strcpy(tmp_val, _("》", "> "));
                target_item_label = i;
            } else
                strcpy(tmp_val, "  ");
        } else if (i <= INVEN_PACK)
            sprintf(tmp_val, "%c)", inven_label[i]);
        else
            sprintf(tmp_val, "%c)", index_to_label(i));

        put_str(tmp_val, j + 1, col);
        cur_col = col + 3;
        if (show_item_graph) {
            TERM_COLOR a = object_attr(o_ptr);
            SYMBOL_CODE c = object_char(o_ptr);
            term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile)
                cur_col++;

            cur_col += 2;
        }

        c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            (void)sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, j + 1, wid - 9);
        }
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    command_gap = col;
    return target_item_label;
}

/*!
 * @brief 所持アイテム一覧を表示する /
 * Choice window "shadow" of the "show_inven()" function
 * @return なし
 */
void display_inventory(player_type *owner_ptr, tval_type tval)
{
    int i, n, z = 0;
    object_type *o_ptr;
    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    TERM_LEN wid, hgt;

    if (!owner_ptr || !owner_ptr->inventory_list)
        return;

    term_get_size(&wid, &hgt);
    for (i = 0; i < INVEN_PACK; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        z = i + 1;
    }

    for (i = 0; i < z; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
        if (item_tester_okay(owner_ptr, o_ptr, tval)) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        term_putstr(0, i, 3, TERM_WHITE, tmp_val);
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        n = strlen(o_name);
        attr = tval_to_attr[o_ptr->tval % 128];
        if (o_ptr->timeout) {
            attr = TERM_L_DARK;
        }

        term_putstr(3, i, n, attr, o_name);
        term_erase(3 + n, i, 255);

        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, i, wid - 9);
        }
    }

    for (i = z; i < hgt; i++)
        term_erase(0, i, 255);
}
