/*!
 * @brief オブジェクトの実装 / Object code, part 1
 * @date 2014/01/10
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "object/object-info.h"
#include "inventory/inventory-slot-types.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "realm/realm-names-table.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "util/int-char-converter.h"

/*!
 * @brief オブジェクト選択時の選択アルファベットラベルを返す /
 * Convert an inventory index into a one character label
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 対応するアルファベット
 * @details Note that the label does NOT distinguish inven/equip.
 */
char index_to_label(int i)
{
    return i < INVEN_MAIN_HAND ? I2A(i) : I2A(i - INVEN_MAIN_HAND);
}

/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
int16_t wield_slot(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    switch (o_ptr->bi_key.tval()) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        if (!player_ptr->inventory_list[INVEN_MAIN_HAND].bi_id) {
            return INVEN_MAIN_HAND;
        }

        if (player_ptr->inventory_list[INVEN_SUB_HAND].bi_id) {
            return INVEN_MAIN_HAND;
        }

        return INVEN_SUB_HAND;
    case ItemKindType::CAPTURE:
    case ItemKindType::CARD:
    case ItemKindType::SHIELD:
        if (!player_ptr->inventory_list[INVEN_SUB_HAND].bi_id) {
            return INVEN_SUB_HAND;
        }

        if (player_ptr->inventory_list[INVEN_MAIN_HAND].bi_id) {
            return INVEN_SUB_HAND;
        }

        return INVEN_MAIN_HAND;
    case ItemKindType::BOW:
        return INVEN_BOW;
    case ItemKindType::RING:
        if (!player_ptr->inventory_list[INVEN_MAIN_RING].bi_id) {
            return INVEN_MAIN_RING;
        }

        return INVEN_SUB_RING;
    case ItemKindType::AMULET:
    case ItemKindType::WHISTLE:
        return INVEN_NECK;
    case ItemKindType::LITE:
        return INVEN_LITE;
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::SOFT_ARMOR:
        return INVEN_BODY;
    case ItemKindType::CLOAK:
        return INVEN_OUTER;
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
        return INVEN_HEAD;
    case ItemKindType::GLOVES:
        return INVEN_ARMS;
    case ItemKindType::BOOTS:
        return INVEN_FEET;
    default:
        return -1;
    }
}

/*!
 * @brief tval/sval指定のベースアイテムがプレイヤーの使用可能な魔法書かどうかを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bi_key ベースアイテム特定キー
 * @return 使用可能な魔法書ならばTRUEを返す。
 */
bool check_book_realm(PlayerType *player_ptr, const BaseitemKey &bi_key)
{
    if (!bi_key.is_spell_book()) {
        return false;
    }

    const auto tval = bi_key.tval();
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::SORCERER)) {
        return is_magic(tval2realm(tval));
    } else if (pc.equals(PlayerClassType::RED_MAGE)) {
        if (is_magic(tval2realm(tval))) {
            return ((tval == ItemKindType::ARCANE_BOOK) || (bi_key.sval() < 2));
        }
    }

    return (get_realm1_book(player_ptr) == tval) || (get_realm2_book(player_ptr) == tval);
}

ItemEntity *ref_item(PlayerType *player_ptr, INVENTORY_IDX i_idx)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    return i_idx >= 0 ? &player_ptr->inventory_list[i_idx] : &(floor_ptr->o_list[0 - i_idx]);
}
