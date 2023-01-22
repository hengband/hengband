#include "view/display-inventory.h"
#include "flavor/flavor-describer.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/string-processor.h"

/*!
 * @brief 所持アイテムの表示を行う /
 * Display the inventory.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 * @details
 * Hack -- do not display "trailing" empty slots
 */
COMMAND_CODE show_inventory(PlayerType *player_ptr, int target_item, BIT_FLAGS mode, const ItemTester &item_tester)
{
    COMMAND_CODE i;
    int k, l, z = 0;
    ItemEntity *o_ptr;
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
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->bi_id) {
            continue;
        }

        z = i + 1;
    }

    prepare_label_string(player_ptr, inven_label, USE_INVEN, item_tester);
    for (k = 0, i = 0; i < z; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!item_tester.okay(o_ptr) && !(mode & USE_FULL)) {
            continue;
        }

        describe_flavor(player_ptr, o_name, o_ptr, 0);
        out_index[k] = i;
        out_color[k] = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
        if (o_ptr->timeout) {
            out_color[k] = TERM_L_DARK;
        }

        (void)strcpy(out_desc[k], o_name);
        l = strlen(out_desc[k]) + 5;
        if (show_weights) {
            l += 9;
        }

        if (show_item_graph) {
            l += 2;
            if (use_bigtile) {
                l++;
            }
        }

        if (l > len) {
            len = l;
        }

        k++;
    }

    col = (len > wid - 4) ? 0 : (wid - len - 1);
    int cur_col;
    int j;
    for (j = 0; j < k; j++) {
        i = out_index[j];
        o_ptr = &player_ptr->inventory_list[i];
        prt("", j + 1, col ? col - 2 : col);
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                angband_strcpy(tmp_val, _("》", "> "), sizeof(tmp_val));
                target_item_label = i;
            } else {
                angband_strcpy(tmp_val, "  ", sizeof(tmp_val));
            }
        } else if (i <= INVEN_PACK) {
            strnfmt(tmp_val, sizeof(tmp_val), "%c)", inven_label[i]);
        } else {
            strnfmt(tmp_val, sizeof(tmp_val), "%c)", index_to_label(i));
        }

        put_str(tmp_val, j + 1, col);
        cur_col = col + 3;
        if (show_item_graph) {
            const auto a = o_ptr->get_color();
            const auto c = o_ptr->get_symbol();
            term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            strnfmt(tmp_val, sizeof(tmp_val), _("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(tmp_val, j + 1, wid - 9);
        }
    }

    if (j && (j < 23)) {
        prt("", j + 1, col ? col - 2 : col);
    }

    command_gap = col;
    return target_item_label;
}

/*!
 * @brief 所持アイテム一覧を表示する /
 * Choice window "shadow" of the "show_inven()" function
 */
void display_inventory(PlayerType *player_ptr, const ItemTester &item_tester)
{
    int i, n, z = 0;
    TERM_COLOR attr = TERM_WHITE;
    char tmp_val[80];
    GAME_TEXT o_name[MAX_NLEN];
    TERM_LEN wid, hgt;

    if (!player_ptr || !player_ptr->inventory_list) {
        return;
    }

    term_get_size(&wid, &hgt);

    for (i = 0; i < INVEN_PACK; i++) {
        auto o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->bi_id) {
            continue;
        }
        z = i + 1;
    }

    for (i = 0; i < z; i++) {
        if (i >= hgt) {
            break;
        }

        auto o_ptr = &player_ptr->inventory_list[i];
        auto do_disp = item_tester.okay(o_ptr);
        angband_strcpy(tmp_val, "   ", sizeof(tmp_val));
        if (do_disp) {
            tmp_val[0] = index_to_label(i);
            tmp_val[1] = ')';
        }

        int cur_col = 3;
        term_erase(cur_col, i, 255);
        term_putstr(0, i, cur_col, TERM_WHITE, tmp_val);
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        n = strlen(o_name);
        attr = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
        if (o_ptr->timeout) {
            attr = TERM_L_DARK;
        }

        if (show_item_graph) {
            const auto a = o_ptr->get_color();
            const auto c = o_ptr->get_symbol();
            term_queue_bigchar(cur_col, i, a, c, 0, 0);
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        term_putstr(cur_col, i, n, attr, o_name);

        if (show_weights) {
            int wgt = o_ptr->weight * o_ptr->number;
            strnfmt(tmp_val, sizeof(tmp_val), _("%3d.%1d kg", "%3d.%1d lb"),
                _(lb_to_kg_integer(wgt), wgt / 10),
                _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(tmp_val, i, wid - 9);
        }
    }

    for (i = z; i < hgt; i++) {
        term_erase(0, i, 255);
    }
}
