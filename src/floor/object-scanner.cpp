﻿#include "floor/object-scanner.h"
#include "floor/cave.h"
#include "flavor/flavor-describer.h"
#include "game-option/text-display-options.h"
#include "grid/grid.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "object/item-tester-hooker.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"

/*!
 * @brief 床に落ちているオブジェクトの数を返す / scan floor items
 * @param items オブジェクトのIDリストを返すための配列参照ポインタ
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param mode オプションフラグ
 * @return 対象のマスに落ちているアイテム数
 * @details
 * Return a list of o_list[] indexes of items at the given floor
 * location. Valid flags are:
 *
 *		mode & 0x01 -- Item tester
 *		mode & 0x02 -- Marked items only
 *		mode & 0x04 -- Stop after first
 */
ITEM_NUMBER scan_floor_items(player_type *owner_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, tval_type item_tester_tval)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return 0;

    OBJECT_IDX this_o_idx, next_o_idx;
    ITEM_NUMBER num = 0;
    for (this_o_idx = floor_ptr->grid_array[y][x].o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if ((mode & SCAN_FLOOR_ITEM_TESTER) && !item_tester_okay(owner_ptr, o_ptr, item_tester_tval))
            continue;

        if ((mode & SCAN_FLOOR_ONLY_MARKED) && !(o_ptr->marked & OM_FOUND))
            continue;

        if (num < 23)
            items[num] = this_o_idx;

        num++;
        if (mode & SCAN_FLOOR_AT_MOST_ONE)
            break;
    }

    return num;
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
static void prepare_label_string_floor(floor_type *floor_ptr, char *label, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
    concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    strcpy(label, alphabet_chars);
    for (int i = 0; i < 52; i++) {
        COMMAND_CODE index;
        SYMBOL_CODE c = alphabet_chars[i];
        if (!get_tag_floor(floor_ptr, &index, c, floor_list, floor_num))
            continue;

        if (label[i] == c)
            label[i] = ' ';

        label[index] = c;
    }
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
COMMAND_CODE show_floor_items(player_type *owner_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, tval_type item_tester_tval)
{
    COMMAND_CODE i, m;
    int j, k, l;
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
    term_get_size(&wid, &hgt);
    int len = MAX((*min_width), 20);
    floor_num = scan_floor_items(owner_ptr, floor_list, y, x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester_tval);
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    for (k = 0, i = 0; i < floor_num && i < 23; i++) {
        o_ptr = &floor_ptr->o_list[floor_list[i]];
        describe_flavor(owner_ptr, o_name, o_ptr, 0);
        out_index[k] = i;
        out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];
        strcpy(out_desc[k], o_name);
        l = strlen(out_desc[k]) + 5;
        if (show_weights)
            l += 9;

        if (o_ptr->tval != TV_GOLD)
            dont_need_to_show_weights = FALSE;

        if (l > len)
            len = l;

        k++;
    }

    if (show_weights && dont_need_to_show_weights)
        len -= 9;

    *min_width = len;
    int col = (len > wid - 4) ? 0 : (wid - len - 1);
    prepare_label_string_floor(floor_ptr, floor_label, floor_list, floor_num);
    for (j = 0; j < k; j++) {
        m = floor_list[out_index[j]];
        o_ptr = &floor_ptr->o_list[m];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                strcpy(tmp_val, _("》", "> "));
                target_item_label = m;
            } else
                strcpy(tmp_val, "   ");
        } else {
            sprintf(tmp_val, "%c)", floor_label[j]);
        }

        put_str(tmp_val, j + 1, col);
        c_put_str(out_color[j], out_desc[j], j + 1, col + 3);
        if (show_weights && (o_ptr->tval != TV_GOLD)) {
            int wgt = o_ptr->weight * o_ptr->number;
            sprintf(tmp_val, _("%3d.%1d kg", "%3d.%1d lb"), _(lbtokg1(wgt), wgt / 10), _(lbtokg2(wgt), wgt % 10));
            prt(tmp_val, j + 1, wid - 9);
        }
    }

    if (j && (j < 23))
        prt("", j + 1, col ? col - 2 : col);

    return target_item_label;
}
