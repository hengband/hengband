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
#include "system/baseitem/baseitem-definition.h"
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
    COMMAND_CODE out_index[23]{};
    TERM_COLOR out_color[23]{};
    std::array<std::string, 23> out_desc{};
    COMMAND_CODE target_item_label = 0;
    auto col = command_gap;
    const auto &[wid, hgt] = term_get_size();
    auto len = wid - col - 1;
    for (i = 0; i < INVEN_PACK; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.is_valid()) {
            continue;
        }

        z = i + 1;
    }

    const auto inven_label = prepare_label_string(player_ptr, USE_INVEN, item_tester);
    for (k = 0, i = 0; i < z; i++) {
        auto &item = *player_ptr->inventory[i];
        if (!item_tester.okay(item) && !(mode & USE_FULL)) {
            continue;
        }

        out_index[k] = i;
        out_color[k] = tval_to_attr[enum2i(item.bi_key.tval()) % 128];
        if (item.timeout) {
            out_color[k] = TERM_L_DARK;
        }

        out_desc[k] = describe_flavor(player_ptr, item, 0);
        l = out_desc[k].length() + 5;
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
        const auto &item = *player_ptr->inventory[i];
        prt("", j + 1, col ? col - 2 : col);
        std::string head;
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                head = _("》", "> ");
                target_item_label = i;
            } else {
                head = "  ";
            }
        } else if (i <= INVEN_PACK) {
            head = format("%c)", inven_label[i]);
        } else {
            head = format("%c)", index_to_label(i));
        }

        put_str(head, j + 1, col);
        cur_col = col + 3;
        if (show_item_graph) {
            term_queue_bigchar(cur_col, j + 1, { item.get_symbol(), {} });
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
        if (show_weights) {
            const auto wgt = item.weight * item.number;
            const auto weight = format(_("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(weight, j + 1, wid - 9);
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
    int i, z = 0;
    TERM_COLOR attr = TERM_WHITE;
    if (!player_ptr || player_ptr->inventory.empty()) {
        return;
    }

    const auto &[wid, hgt] = term_get_size();
    for (i = 0; i < INVEN_PACK; i++) {
        auto o_ptr = player_ptr->inventory[i].get();
        if (!o_ptr->is_valid()) {
            continue;
        }
        z = i + 1;
    }

    for (i = 0; i < z; i++) {
        if (i >= hgt) {
            break;
        }

        auto &item = *player_ptr->inventory[i];
        auto do_disp = item_tester.okay(item);
        std::string label = "   ";
        if (do_disp) {
            label[0] = index_to_label(i);
            label[1] = ')';
        }

        int cur_col = 3;
        term_erase(cur_col, i);
        term_putstr(0, i, cur_col, TERM_WHITE, label);
        const auto item_name = describe_flavor(player_ptr, item, 0);
        attr = tval_to_attr[enum2i(item.bi_key.tval()) % 128];
        if (item.timeout) {
            attr = TERM_L_DARK;
        }

        if (show_item_graph) {
            term_queue_bigchar(cur_col, i, { item.get_symbol(), {} });
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        term_putstr(cur_col, i, item_name.length(), attr, item_name);

        if (show_weights) {
            const auto wgt = item.weight * item.number;
            const auto weight = format(_("%3d.%1d kg", "%3d.%1d lb"),
                _(lb_to_kg_integer(wgt), wgt / 10),
                _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(weight, i, wid - 9);
        }
    }

    for (i = z; i < hgt; i++) {
        term_erase(0, i);
    }
}
