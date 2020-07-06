#include "inventory/player-inventory.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "floor/floor-object.h"
#include "floor/object-scanner.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flavor.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player/player-move.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

/*!
 * @brief 規定の処理にできるアイテムがプレイヤーの利用可能範囲内にあるかどうかを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(player_type *owner_ptr, tval_type tval)
{
    for (int j = 0; j < INVEN_TOTAL; j++)
        if (item_tester_okay(owner_ptr, &owner_ptr->inventory_list[j], tval))
            return TRUE;

    OBJECT_IDX floor_list[23];
    ITEM_NUMBER floor_num = scan_floor_items(owner_ptr, floor_list, owner_ptr->y, owner_ptr->x, 0x03, tval);
    return floor_num != 0;
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(player_type *owner_ptr)
{
    OBJECT_IDX this_o_idx;
    OBJECT_IDX item;
    item_tester_hook = check_store_item_to_inventory;
    concptr q = _("どれを拾いますか？", "Get which item? ");
    concptr s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");
    if (choose_object(owner_ptr, &item, q, s, (USE_FLOOR), 0))
        this_o_idx = 0 - item;
    else
        return FALSE;

    py_pickup_aux(owner_ptr, this_o_idx);
    return TRUE;
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @return なし
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(player_type *owner_ptr, bool pickup)
{
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    int floor_num = 0;
    OBJECT_IDX floor_o_idx = 0;
    int can_pickup = 0;
    for (this_o_idx = owner_ptr->current_floor_ptr->grid_array[owner_ptr->y][owner_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        o_ptr = &owner_ptr->current_floor_ptr->o_list[this_o_idx];
        object_desc(owner_ptr, o_name, o_ptr, 0);
        next_o_idx = o_ptr->next_o_idx;
        disturb(owner_ptr, FALSE, FALSE);
        if (o_ptr->tval == TV_GOLD) {
            msg_format(_(" $%ld の価値がある%sを見つけた。", "You have found %ld gold pieces worth of %s."), (long)o_ptr->pval, o_name);
            owner_ptr->au += o_ptr->pval;
            owner_ptr->redraw |= (PR_GOLD);
            owner_ptr->window |= (PW_PLAYER);
            delete_object_idx(owner_ptr, this_o_idx);
            continue;
        } else if (o_ptr->marked & OM_NOMSG) {
            o_ptr->marked &= ~(OM_NOMSG);
            continue;
        }

        if (check_store_item_to_inventory(owner_ptr, o_ptr))
            can_pickup++;

        floor_num++;
        floor_o_idx = this_o_idx;
    }

    if (!floor_num)
        return;

    if (!pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            object_desc(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("%sがある。", "You see %s."), o_name);
        } else
            msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);

        return;
    }

    if (!can_pickup) {
        if (floor_num == 1) {
            o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
            object_desc(owner_ptr, o_name, o_ptr, 0);
            msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
        } else
            msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

        return;
    }

    if (floor_num != 1) {
        while (can_pickup--)
            if (!py_pickup_floor_aux(owner_ptr))
                break;

        return;
    }

    if (carry_query_flag) {
        char out_val[MAX_NLEN + 20];
        o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
        object_desc(owner_ptr, o_name, o_ptr, 0);
        (void)sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);
        if (!get_check(out_val))
            return;
    }

    o_ptr = &owner_ptr->current_floor_ptr->o_list[floor_o_idx];
    py_pickup_aux(owner_ptr, floor_o_idx);
}

/*!
 * @brief 装備アイテムの表示を行う /
 * Display the equipment.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 */
COMMAND_CODE show_equipment(player_type *owner_ptr, int target_item, BIT_FLAGS mode, tval_type tval)
{
    COMMAND_CODE i;
    int j, k, l;
    object_type *o_ptr;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    char out_desc[23][MAX_NLEN];
    COMMAND_CODE target_item_label = 0;
    TERM_LEN wid, hgt;
    char equip_label[52 + 1];
    int col = command_gap;
    Term_get_size(&wid, &hgt);
    int len = wid - col - 1;
    for (k = 0, i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!(select_ring_slot ? is_ring_slot(i) : item_tester_okay(owner_ptr, o_ptr, tval) || (mode & USE_FULL))
            && (!((((i == INVEN_RARM) && owner_ptr->hidarite) || ((i == INVEN_LARM) && owner_ptr->migite)) && owner_ptr->ryoute)
                || (mode & IGNORE_BOTHHAND_SLOT)))
            continue;

        object_desc(owner_ptr, o_name, o_ptr, 0);
        if ((((i == INVEN_RARM) && owner_ptr->hidarite) || ((i == INVEN_LARM) && owner_ptr->migite)) && owner_ptr->ryoute) {
            (void)strcpy(out_desc[k], _("(武器を両手持ち)", "(wielding with two-hands)"));
            out_color[k] = TERM_WHITE;
        } else {
            (void)strcpy(out_desc[k], o_name);
            out_color[k] = tval_to_attr[o_ptr->tval % 128];
        }

        out_index[k] = i;
        if (o_ptr->timeout)
            out_color[k] = TERM_L_DARK;
        l = strlen(out_desc[k]) + (2 + _(1, 3));

        if (show_labels)
            l += (_(7, 14) + 2);

        if (show_weights)
            l += 9;

        if (show_item_graph)
            l += 2;

        if (l > len)
            len = l;

        k++;
    }

    col = (len > wid - _(6, 4)) ? 0 : (wid - len - 1);
    prepare_label_string(owner_ptr, equip_label, USE_EQUIP, tval);
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
        } else if (i >= INVEN_RARM)
            sprintf(tmp_val, "%c)", equip_label[i - INVEN_RARM]);
        else
            sprintf(tmp_val, "%c)", index_to_label(i));

        put_str(tmp_val, j + 1, col);
        int cur_col = col + 3;
        if (show_item_graph) {
            TERM_COLOR a = object_attr(o_ptr);
            SYMBOL_CODE c = object_char(o_ptr);
            Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile)
                cur_col++;

            cur_col += 2;
        }

        if (show_labels) {
            (void)sprintf(tmp_val, _("%-7s: ", "%-14s: "), mention_use(owner_ptr, i));
            put_str(tmp_val, j + 1, cur_col);
            c_put_str(out_color[j], out_desc[j], j + 1, _(cur_col + 9, cur_col + 16));
        } else
            c_put_str(out_color[j], out_desc[j], j + 1, cur_col);

        if (!show_weights)
            continue;

        int wgt = o_ptr->weight * o_ptr->number;
        (void)sprintf(tmp_val, _("%3d.%1d kg", "%3d.%d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
        prt(tmp_val, j + 1, wid - 9);
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    command_gap = col;
    return target_item_label;
}
