#include "window/main-window-equipments.h"
#include "flavor/flavor-describer.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "player/player-status-flags.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <array>
#include <vector>

/*!
 * @brief メインウィンドウの右上に装備アイテムの表示させる
 * Display the equipment.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 */
COMMAND_CODE show_equipment(PlayerType *player_ptr, int target_item, BIT_FLAGS mode, const ItemTester &item_tester)
{
    COMMAND_CODE i;
    int j, k, l;
    ItemEntity *o_ptr;
    char tmp_val[80];
    COMMAND_CODE out_index[23];
    TERM_COLOR out_color[23];
    std::array<std::string, 23> out_desc{};
    COMMAND_CODE target_item_label = 0;
    TERM_LEN wid, hgt;
    char equip_label[52 + 1];
    int col = command_gap;
    term_get_size(&wid, &hgt);
    int len = wid - col - 1;
    for (k = 0, i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        auto only_slot = !(player_ptr->select_ring_slot ? is_ring_slot(i) : (item_tester.okay(o_ptr) || any_bits(mode, USE_FULL)));
        auto is_any_hand = (i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr);
        is_any_hand |= (i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr);
        auto is_two_handed = is_any_hand && has_two_handed_weapons(player_ptr);
        only_slot &= !is_two_handed || any_bits(mode, IGNORE_BOTHHAND_SLOT);
        if (only_slot) {
            continue;
        }

        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
        if (is_two_handed) {
            out_desc[k] = _("(武器を両手持ち)", "(wielding with two-hands)");
            out_color[k] = TERM_WHITE;
        } else {
            out_desc[k] = item_name;
            out_color[k] = tval_to_attr[enum2i(o_ptr->bi_key.tval()) % 128];
        }

        out_index[k] = i;
        if (o_ptr->timeout) {
            out_color[k] = TERM_L_DARK;
        }
        l = out_desc[k].length() + (2 + _(1, 3));

        if (show_labels) {
            l += (_(7, 14) + 2);
        }

        if (show_weights) {
            l += 9;
        }

        if (show_item_graph) {
            l += 2;
        }

        if (l > len) {
            len = l;
        }

        k++;
    }

    col = (len > wid - _(6, 4)) ? 0 : (wid - len - 1);
    prepare_label_string(player_ptr, equip_label, USE_EQUIP, item_tester);
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
        } else if (i >= INVEN_MAIN_HAND) {
            strnfmt(tmp_val, sizeof(tmp_val), "%c)", equip_label[i - INVEN_MAIN_HAND]);
        } else {
            strnfmt(tmp_val, sizeof(tmp_val), "%c)", index_to_label(i));
        }

        put_str(tmp_val, j + 1, col);
        int cur_col = col + 3;
        if (show_item_graph) {
            const auto a = o_ptr->get_color();
            const auto c = o_ptr->get_symbol();
            term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
            if (use_bigtile) {
                cur_col++;
            }

            cur_col += 2;
        }

        if (show_labels) {
            strnfmt(tmp_val, sizeof(tmp_val), _("%-7s: ", "%-14s: "), mention_use(player_ptr, i));
            put_str(tmp_val, j + 1, cur_col);
            c_put_str(out_color[j], out_desc[j], j + 1, _(cur_col + 9, cur_col + 16));
        } else {
            c_put_str(out_color[j], out_desc[j], j + 1, cur_col);
        }

        if (!show_weights) {
            continue;
        }

        int wgt = o_ptr->weight * o_ptr->number;
        strnfmt(tmp_val, sizeof(tmp_val), _("%3d.%1d kg", "%3d.%d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
        prt(tmp_val, j + 1, wid - 9);
    }

    if (j && (j < 23)) {
        prt("", j + 1, col ? col - 2 : col);
    }

    command_gap = col;
    return target_item_label;
}
