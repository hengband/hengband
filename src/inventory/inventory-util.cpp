/*!
 * @brief インベントリ関係のユーティリティ
 * @date 2020/07/02
 * @author Hourier
 * @details 少々雑多なので後で整理を検討する
 */

#include "inventory/inventory-util.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include <sstream>

/*!
 * @brief プレイヤーの所持/装備オブジェクトIDが指輪枠かを返す /
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 指輪枠ならばTRUEを返す。
 */
bool is_ring_slot(int i)
{
    return (i == INVEN_MAIN_RING) || (i == INVEN_SUB_RING);
}

/*!
 * @brief 床上のアイテムで該当するタグのある要素番号を返す
 * @param floor フロアへの参照
 * @param tag 該当するオブジェクトがあるかを調べたいタグ
 * @param floor_item_index 床上アイテムの配列
 * @return タグに該当するアイテムがあればfloor_item_indexの要素番号、なければnullopt
 */
tl::optional<short> get_tag_floor(const FloorType &floor, char tag, const std::vector<short> &floor_item_index)
{
    tl::optional<short> floor_item_indice;
    for (size_t i = 0; i < floor_item_index.size(); i++) {
        const auto &item = *floor.o_list[floor_item_index[i]];
        if (!item.is_inscribed()) {
            continue;
        }

        auto s = extract_suffix(*item.inscription, '@');
        while (s) {
            // コマンドと同じタグならそれで決まり.
            if ((s->length() > 2) && (s->at(1) == command_cmd) && (s->at(2) == tag)) {
                return static_cast<short>(i);
            }

            // その他のタグは一旦保持して次のタグを探す.
            if (!floor_item_indice && is_numeric(tag) && (s->length() > 1) && (s->at(1) == tag)) {
                floor_item_indice = static_cast<short>(i);
            }

            s = extract_suffix(s->substr(1), '@');
        }
    }

    return floor_item_indice;
}

static tl::optional<std::pair<short, short>> get_inventory_range(BIT_FLAGS mode)
{
    switch (mode) {
    case USE_EQUIP:
        return std::make_pair(INVEN_MAIN_HAND, INVEN_TOTAL);
    case USE_INVEN:
        return std::make_pair(static_cast<short>(0), INVEN_PACK);
    default:
        return tl::nullopt;
    }
}

/*!
 * @brief 所持/装備オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す
 * @param player_ptr プレイヤーへの参照ポインタ
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
bool get_tag(PlayerType *player_ptr, COMMAND_CODE *cp, char tag, BIT_FLAGS mode, const ItemTester &item_tester)
{
    const auto range = get_inventory_range(mode);
    if (!range) {
        return false;
    }

    const auto &[start, end] = *range;
    for (auto i = start; i < end; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.is_valid() || !item.is_inscribed()) {
            continue;
        }

        if (!item_tester.okay(&item) && !(mode & USE_FULL)) {
            continue;
        }

        auto s = extract_suffix(*item.inscription, '@');
        while (s) {
            if ((s->length() > 2) && (s->at(1) == command_cmd) && (s->at(2) == tag)) {
                *cp = i;
                return true;
            }

            s = extract_suffix(s->substr(1), '@');
        }
    }

    if (!is_numeric(tag)) {
        return false;
    }

    for (auto i = start; i < end; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.is_valid() || !item.is_inscribed()) {
            continue;
        }

        if (!item_tester.okay(&item) && !(mode & USE_FULL)) {
            continue;
        }

        auto s = extract_suffix(*item.inscription, '@');
        while (s) {
            if ((s->length() > 1) && (s->at(1) == tag)) {
                *cp = i;
                return true;
            }

            s = extract_suffix(s->substr(1), '@');
        }
    }

    return false;
}

/*!
 * @brief プレイヤーの所持/装備オブジェクトが正規のものかを返す /
 * Auxiliary function for "get_item()" -- test an index
 * @param i 選択アイテムID
 * @return 正規のIDならばTRUEを返す。
 */
bool get_item_okay(PlayerType *player_ptr, OBJECT_IDX i, const ItemTester &item_tester)
{
    if ((i < 0) || (i >= INVEN_TOTAL)) {
        return false;
    }

    if (player_ptr->select_ring_slot) {
        return is_ring_slot(i);
    }

    return item_tester.okay(player_ptr->inventory[i].get());
}

/*!
 * @brief 選択したアイテムの確認処理のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 */
bool get_item_allow(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    if (!command_cmd) {
        return true;
    }

    ItemEntity *o_ptr;
    if (i_idx >= 0) {
        o_ptr = player_ptr->inventory[i_idx].get();
    } else {
        o_ptr = player_ptr->current_floor_ptr->o_list[0 - i_idx].get();
    }

    if (!o_ptr->is_inscribed()) {
        return true;
    }

    auto s = angband_strchr(o_ptr->inscription->data(), '!');
    while (s) {
        if ((s[1] == command_cmd) || (s[1] == '*')) {
            if (!verify(player_ptr, _("本当に", "Really try"), i_idx)) {
                return false;
            }
        }

        s = angband_strchr(s + 1, '!');
    }

    return true;
}

/*!
 * @brief 選択アルファベットラベルからプレイヤーの装備オブジェクトIDを返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Convert a label into the index of a item in the "equip"
 * @return 対応するID。該当スロットにオブジェクトが存在しなかった場合-1を返す / Return "-1" if the label does not indicate a real item
 */
INVENTORY_IDX label_to_equipment(PlayerType *player_ptr, int c)
{
    INVENTORY_IDX i = (INVENTORY_IDX)(islower(c) ? A2I(c) : -1) + INVEN_MAIN_HAND;

    if ((i < INVEN_MAIN_HAND) || (i >= INVEN_TOTAL)) {
        return -1;
    }

    if (player_ptr->select_ring_slot) {
        return is_ring_slot(i) ? i : -1;
    }

    if (!player_ptr->inventory[i]->bi_id) {
        return -1;
    }

    return i;
}

/*!
 * @brief 選択アルファベットラベルからプレイヤーの所持オブジェクトIDを返す /
 * Convert a label into the index of an item in the "inven"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param c 選択されたアルファベット
 * @return 対応するID。該当スロットにオブジェクトが存在しなかった場合-1を返す / Return "-1" if the label does not indicate a real item
 * @details Note that the label does NOT distinguish inven/equip.
 */
INVENTORY_IDX label_to_inventory(PlayerType *player_ptr, int c)
{
    INVENTORY_IDX i = (INVENTORY_IDX)(islower(c) ? A2I(c) : -1);

    if ((i < 0) || (i > INVEN_PACK) || !player_ptr->inventory[i]->is_valid()) {
        return -1;
    }

    return i;
}

/*!
 * @brief 選択したアイテムの確認処理の補助
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prompt メッセージ表示の一部
 * @param i_idx 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 */
bool verify(PlayerType *player_ptr, concptr prompt, INVENTORY_IDX i_idx)
{
    const auto &item = i_idx >= 0 ? *player_ptr->inventory[i_idx] : *player_ptr->current_floor_ptr->o_list[0 - i_idx];
    const auto item_name = describe_flavor(player_ptr, item, 0);
    std::stringstream ss;
    ss << prompt;
#ifndef JP
    ss << ' ';
#endif
    ss << item_name << _("ですか? ", "? ");
    return input_check(ss.str());
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mode 所持品リストか装備品リストかの切り替え
 * @param item_tester アイテムの絞り込み条件
 * @return 有効なラベルリスト
 */
std::string prepare_label_string(PlayerType *player_ptr, BIT_FLAGS mode, const ItemTester &item_tester)
{
    constexpr std::string_view alphabet("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    std::string tag_chars(alphabet);
    const auto offset = match_bits(mode, USE_EQUIP, USE_EQUIP) ? INVEN_MAIN_HAND : 0;
    for (size_t i = 0; i < tag_chars.length(); i++) {
        short i_idx;
        const auto tag_char = alphabet[i];
        if (!get_tag(player_ptr, &i_idx, tag_char, mode, item_tester)) {
            continue;
        }

        if (tag_chars[i] == tag_char) {
            tag_chars[i] = ' ';
        }

        tag_chars[i_idx - offset] = tag_char;
    }

    return tag_chars;
}
