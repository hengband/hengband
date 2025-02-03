#include "floor/object-scanner.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "object/item-tester-hooker.h"
#include "object/object-mark-types.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/string-processor.h"
#include <array>

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
int scan_floor_items(PlayerType *player_ptr, OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode, const ItemTester &item_tester)
{
    const Pos2D pos(y, x);
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!in_bounds(floor, pos.y, pos.x)) {
        return 0;
    }

    auto num = 0;
    for (const auto this_o_idx : floor.get_grid(pos).o_idx_list) {
        const auto &item = floor.o_list[this_o_idx];
        if ((mode & SCAN_FLOOR_ITEM_TESTER) && !item_tester.okay(&item)) {
            continue;
        }

        if ((mode & SCAN_FLOOR_ONLY_MARKED) && item.marked.has_not(OmType::FOUND)) {
            continue;
        }

        if (num < 23) {
            items[num] = this_o_idx;
        }

        num++;
        if (mode & SCAN_FLOOR_AT_MOST_ONE) {
            break;
        }
    }

    return num;
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す(床上アイテム用) /
 * Move around label characters with correspond tags (floor version)
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 */
/*
 */
static void prepare_label_string_floor(const FloorType &floor, char *label, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
    concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    strcpy(label, alphabet_chars);
    for (int i = 0; i < 52; i++) {
        COMMAND_CODE index;
        auto c = alphabet_chars[i];
        if (!get_tag_floor(floor, &index, c, floor_list, floor_num)) {
            continue;
        }

        if (label[i] == c) {
            label[i] = ' ';
        }

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
COMMAND_CODE show_floor_items(PlayerType *player_ptr, int target_item, POSITION y, POSITION x, TERM_LEN *min_width, const ItemTester &item_tester)
{
    COMMAND_CODE i, m;
    int j, k, l;
    COMMAND_CODE out_index[23]{};
    TERM_COLOR out_color[23]{};
    std::array<std::string, 23> descriptions{};
    COMMAND_CODE target_item_label = 0;
    OBJECT_IDX floor_list[23]{};
    ITEM_NUMBER floor_num;
    char floor_label[52 + 1]{};
    auto dont_need_to_show_weights = true;
    const auto &[wid, hgt] = term_get_size();
    auto len = std::max((*min_width), 20);
    floor_num = scan_floor_items(player_ptr, floor_list, y, x, SCAN_FLOOR_ITEM_TESTER | SCAN_FLOOR_ONLY_MARKED, item_tester);
    auto &floor = *player_ptr->current_floor_ptr;
    for (k = 0, i = 0; i < floor_num && i < 23; i++) {
        const auto &item = floor.o_list[floor_list[i]];
        const auto item_name = describe_flavor(player_ptr, item, 0);
        out_index[k] = i;
        const auto tval = item.bi_key.tval();
        out_color[k] = tval_to_attr[enum2i(tval) & 0x7F];
        descriptions[k] = item_name;
        l = descriptions[k].length() + 5;
        if (show_weights) {
            l += 9;
        }

        if (tval != ItemKindType::GOLD) {
            dont_need_to_show_weights = false;
        }

        if (l > len) {
            len = l;
        }

        k++;
    }

    if (show_weights && dont_need_to_show_weights) {
        len -= 9;
    }

    *min_width = len;
    int col = (len > wid - 4) ? 0 : (wid - len - 1);
    prepare_label_string_floor(floor, floor_label, floor_list, floor_num);
    for (j = 0; j < k; j++) {
        m = floor_list[out_index[j]];
        const auto &item = floor.o_list[m];
        prt("", j + 1, col ? col - 2 : col);
        std::string head;
        if (use_menu && target_item) {
            if (j == (target_item - 1)) {
                head = _("》", "> ");
                target_item_label = m;
            } else {
                head = "   ";
            }
        } else {
            head = format("%c)", floor_label[j]);
        }

        put_str(head, j + 1, col);
        c_put_str(out_color[j], descriptions[j], j + 1, col + 3);
        if (show_weights && (item.bi_key.tval() != ItemKindType::GOLD)) {
            int wgt = item.weight * item.number;
            const auto weight = format(_("%3d.%1d kg", "%3d.%1d lb"), _(lb_to_kg_integer(wgt), wgt / 10), _(lb_to_kg_fraction(wgt), wgt % 10));
            prt(weight, j + 1, wid - 9);
        }
    }

    if (j && (j < 23)) {
        prt("", j + 1, col ? col - 2 : col);
    }

    return target_item_label;
}
