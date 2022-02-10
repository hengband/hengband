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
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/string-processor.h"

/*!
 * @brief プレイヤーの所持/装備オブジェクトIDが指輪枠かを返す /
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 指輪枠ならばTRUEを返す。
 */
bool is_ring_slot(int i) { return (i == INVEN_MAIN_RING) || (i == INVEN_SUB_RING); }

/*!
 * @brief 床オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" inventory object with the given "tag".
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
bool get_tag_floor(floor_type *floor_ptr, COMMAND_CODE *cp, char tag, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
    for (COMMAND_CODE i = 0; i < floor_num && i < 23; i++) {
        auto *o_ptr = &floor_ptr->o_list[floor_list[i]];
        if (!o_ptr->inscription)
            continue;

        concptr s = angband_strchr(quark_str(o_ptr->inscription), '@');
        while (s) {
            if ((s[1] == command_cmd) && (s[2] == tag)) {
                *cp = i;
                return true;
            }

            s = angband_strchr(s + 1, '@');
        }
    }

    if (tag < '0' || '9' < tag) {
        return false;
    }

    for (COMMAND_CODE i = 0; i < floor_num && i < 23; i++) {
        auto *o_ptr = &floor_ptr->o_list[floor_list[i]];
        if (!o_ptr->inscription)
            continue;

        concptr s = angband_strchr(quark_str(o_ptr->inscription), '@');
        while (s) {
            if (s[1] == tag) {
                *cp = i;
                return true;
            }

            s = angband_strchr(s + 1, '@');
        }
    }

    return false;
}

/*!
 * @brief 所持/装備オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" inventory object with the given "tag".
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
bool get_tag(PlayerType *player_ptr, COMMAND_CODE *cp, char tag, BIT_FLAGS mode, const ItemTester& item_tester)
{
    COMMAND_CODE start, end;
    switch (mode) {
    case USE_EQUIP:
        start = INVEN_MAIN_HAND;
        end = INVEN_TOTAL - 1;
        break;

    case USE_INVEN:
        start = 0;
        end = INVEN_PACK - 1;
        break;

    default:
        return false;
    }

    for (COMMAND_CODE i = start; i <= end; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->k_idx == 0) || (o_ptr->inscription == 0))
            continue;

        if (!item_tester.okay(o_ptr) && !(mode & USE_FULL))
            continue;

        concptr s = angband_strchr(quark_str(o_ptr->inscription), '@');
        while (s) {
            if ((s[1] == command_cmd) && (s[2] == tag)) {
                *cp = i;
                return true;
            }

            s = angband_strchr(s + 1, '@');
        }
    }

    if (tag < '0' || '9' < tag)
        return false;

    for (COMMAND_CODE i = start; i <= end; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if ((o_ptr->k_idx == 0) || (o_ptr->inscription == 0))
            continue;

        if (!item_tester.okay(o_ptr) && !(mode & USE_FULL))
            continue;

        concptr s = angband_strchr(quark_str(o_ptr->inscription), '@');
        while (s) {
            if (s[1] == tag) {
                *cp = i;
                return true;
            }

            s = angband_strchr(s + 1, '@');
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
bool get_item_okay(PlayerType *player_ptr, OBJECT_IDX i, const ItemTester& item_tester)
{
    if ((i < 0) || (i >= INVEN_TOTAL))
        return false;

    if (player_ptr->select_ring_slot)
        return is_ring_slot(i);

    return item_tester.okay(&player_ptr->inventory_list[i]);
}

/*!
 * @brief 選択したアイテムの確認処理のメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 * Hack -- allow user to "prevent" certain choices
 */
bool get_item_allow(PlayerType *player_ptr, INVENTORY_IDX item)
{
    if (!command_cmd)
        return true;

    ObjectType *o_ptr;
    if (item >= 0)
        o_ptr = &player_ptr->inventory_list[item];
    else
        o_ptr = &player_ptr->current_floor_ptr->o_list[0 - item];

    if (!o_ptr->inscription)
        return true;

    concptr s = angband_strchr(quark_str(o_ptr->inscription), '!');
    while (s) {
        if ((s[1] == command_cmd) || (s[1] == '*'))
            if (!verify(player_ptr, _("本当に", "Really try"), item))
                return false;

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

    if ((i < INVEN_MAIN_HAND) || (i >= INVEN_TOTAL))
        return -1;

    if (player_ptr->select_ring_slot)
        return is_ring_slot(i) ? i : -1;

    if (!player_ptr->inventory_list[i].k_idx)
        return -1;

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

    if ((i < 0) || (i > INVEN_PACK) || (player_ptr->inventory_list[i].k_idx == 0))
        return -1;

    return i;
}

/*!
 * @brief 選択したアイテムの確認処理の補助 /
 * Verify the choice of an item.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prompt メッセージ表示の一部
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 */
bool verify(PlayerType *player_ptr, concptr prompt, INVENTORY_IDX item)
{
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[MAX_NLEN + 20];
    ObjectType *o_ptr;
    if (item >= 0)
        o_ptr = &player_ptr->inventory_list[item];
    else
        o_ptr = &player_ptr->current_floor_ptr->o_list[0 - item];

    describe_flavor(player_ptr, o_name, o_ptr, 0);
    (void)sprintf(out_val, _("%s%sですか? ", "%s %s? "), prompt, o_name);
    return get_check(out_val);
}

/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す /
 * Move around label characters with correspond tags
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param mode 所持品リストか装備品リストかの切り替え
 */
void prepare_label_string(PlayerType *player_ptr, char *label, BIT_FLAGS mode, const ItemTester& item_tester)
{
    concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int offset = (mode == USE_EQUIP) ? INVEN_MAIN_HAND : 0;
    strcpy(label, alphabet_chars);
    for (int i = 0; i < 52; i++) {
        COMMAND_CODE index;
        SYMBOL_CODE c = alphabet_chars[i];
        if (!get_tag(player_ptr, &index, c, mode, item_tester))
            continue;

        if (label[i] == c)
            label[i] = ' ';

        label[index - offset] = c;
    }
}
