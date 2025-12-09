#include "floor/object-scanner.h"
#include "flavor/flavor-describer.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-util.h"
#include "io/input-key-requester.h"
#include "locale/japanese.h"
#include "object/item-tester-hooker.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include <array>

/*!
 * @brief 床に落ちているオブジェクトのインデックス群を返す
 * @param floor フロアへの参照
 * @param pos 走査するフロアの座標
 * @param mode オプションフラグ
 * @return 対象のマスに落ちているアイテム数
 */
std::vector<short> scan_floor_items(const FloorType &floor, const Pos2D &pos, const EnumClassFlagGroup<ScanFloorMode> &mode, const ItemTester &item_tester)
{
    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_EXCLUSIVE)) {
        return {};
    }

    std::vector<short> items;
    for (const auto this_o_idx : floor.get_grid(pos).o_idx_list) {
        const auto &item = *floor.o_list[this_o_idx];
        if (mode.has(ScanFloorMode::ITEM_TESTER) && !item_tester.okay(&item)) {
            continue;
        }

        if (mode.has(ScanFloorMode::ONLY_MARKED) && item.marked.has_not(OmType::FOUND)) {
            continue;
        }

        items.push_back(this_o_idx);
        if (mode.has(ScanFloorMode::AT_MOST_ONE)) {
            break;
        }
    }

    return items;
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す(床上アイテム用)
 * @param floor フロアへの参照
 * @param floor_item_index 床上アイテムインデックス群
 * @return タグアルファベットのリスト
 */
static std::string prepare_label_string_floor(const FloorType &floor, const std::vector<short> &floor_item_index)
{
    constexpr std::string_view alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    std::string tag_chars(alphabet);
    for (size_t i = 0; i < tag_chars.length(); i++) {
        const auto tag_char = alphabet[i];
        const auto fii_num = get_tag_floor(floor, tag_char, floor_item_index);
        if (!fii_num) {
            continue;
        }

        if (tag_chars[i] == tag_char) {
            tag_chars[i] = ' ';
        }

        tag_chars[*fii_num] = tag_char;
    }

    return tag_chars;
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
    const Pos2D pos(y, x);
    constexpr auto max_items = 23; //!< @todo 1マスに落ちているアイテムの最大数. ヘッダに移したい.
    COMMAND_CODE m;
    int j, l;
    COMMAND_CODE out_index[max_items]{};
    TERM_COLOR out_color[max_items]{};
    std::array<std::string, max_items> descriptions{};
    COMMAND_CODE target_item_label = 0;
    auto dont_need_to_show_weights = true;
    const auto &[wid, hgt] = term_get_size();
    auto len = std::max((*min_width), 20);
    auto &floor = *player_ptr->current_floor_ptr;
    auto floor_item_index = scan_floor_items(floor, pos, { ScanFloorMode::ITEM_TESTER, ScanFloorMode::ONLY_MARKED }, item_tester);
    auto k = 0;
    for (size_t i = 0; (i < floor_item_index.size()) && (i < max_items); i++) {
        const auto &item = *floor.o_list[floor_item_index[i]];
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
    const auto floor_label = prepare_label_string_floor(floor, floor_item_index);
    for (j = 0; j < k; j++) {
        m = floor_item_index[out_index[j]];
        const auto &item = *floor.o_list[m];
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
            head = fmt::format("{})", floor_label[j]);
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
