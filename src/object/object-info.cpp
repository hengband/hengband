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
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object-enchant/object-ego.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "realm/realm-names-table.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include <sstream>

/*!
 * @brief アイテムの発動効果名称を返す（サブルーチン/ブレス）
 * @param item アイテムへの参照
 * @return std::string 発動名称
 */
static std::string item_activation_dragon_breath(const ItemEntity &item)
{
    std::string desc = _("", "breathe ");
    auto n = 0;
    const auto flags = item.get_flags();
    for (auto i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (flags.has(dragonbreath_info[i].flag)) {
            if (n > 0) {
                desc.append(_("、", ", "));
            }

            desc.append(dragonbreath_info[i].name);
            n++;
        }
    }

    desc.append(_("のブレス(250)", " (250)"));
    return desc;
}

static std::string build_activation_description(const activation_type &act, const ItemEntity &item)
{
    switch (act.index) {
    case RandomArtActType::NONE:
        return act.desc;
    case RandomArtActType::BR_FIRE:
        if (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES)) {
            return _("火炎のブレス (200) と火への耐性", "breathe fire (200) and resist fire");
        }

        return act.desc;
    case RandomArtActType::BR_COLD:
        if (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE)) {
            return _("冷気のブレス (200) と冷気への耐性", "breathe cold (200) and resist cold");
        }

        return act.desc;
    case RandomArtActType::BR_DRAGON:
        return item_activation_dragon_breath(item);
    case RandomArtActType::AGGRAVATE:
        if (item.is_specific_artifact(FixedArtifactId::HYOUSIGI)) {
            return _("拍子木を打ちならす", "beat wooden clappers");
        }

        return act.desc;
    case RandomArtActType::ACID_BALL_AND_RESISTANCE:
        return _("アシッド・ボール (100) と酸への耐性", "ball of acid (100) and resist acid");
    case RandomArtActType::FIRE_BALL_AND_RESISTANCE:
        return _("ファイア・ボール (100) と火への耐性", "ball of fire (100) and resist fire");
    case RandomArtActType::COLD_BALL_AND_RESISTANCE:
        return _("アイス・ボール (100) と冷気への耐性", "ball of cold (100) and resist cold");
    case RandomArtActType::ELEC_BALL_AND_RESISTANCE:
        return _("サンダー・ボール (100) と電撃への耐性", "ball of elec (100) and resist elec");
    case RandomArtActType::POIS_BALL_AND_RESISTANCE:
        return _("ポイズン・ボール (100) と毒への耐性", "ball of poison (100) and resist elec");
    case RandomArtActType::RESIST_ACID:
        return _("一時的な酸への耐性", "temporary resist acid");
    case RandomArtActType::RESIST_FIRE:
        return _("一時的な火への耐性", "temporary resist fire");
    case RandomArtActType::RESIST_COLD:
        return _("一時的な冷気への耐性", "temporary resist cold");
    case RandomArtActType::RESIST_ELEC:
        return _("一時的な電撃への耐性", "temporary resist elec");
    case RandomArtActType::RESIST_POIS:
        return _("一時的な毒への耐性", "temporary resist elec");
    default:
        return act.desc;
    }
}

static std::string build_timeout_description(const activation_type &act, const ItemEntity &item)
{
    const auto constant = act.timeout.constant;
    const auto dice = act.timeout.dice;
    if (constant == 0 && dice == 0) {
        return _("いつでも", "every turn");
    }

    if (constant >= 0) {
        std::stringstream ss;
        ss << _("", "every ");
        if (constant > 0) {
            ss << constant;
            if (dice > 0) {
                ss << '+';
            }
        }

        if (dice > 0) {
            ss << 'd' << dice;
        }

        ss << _(" ターン毎", " turns");
        return ss.str();
    }

    std::stringstream ss;
    switch (act.index) {
    case RandomArtActType::BR_FIRE:
        ss << _("", "every ") << (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_FLAMES) ? 200 : 250) << _(" ターン毎", " turns");
        return ss.str();
    case RandomArtActType::BR_COLD:
        ss << _("", "every ") << (item.bi_key == BaseitemKey(ItemKindType::RING, SV_RING_ICE) ? 200 : 250) << _(" ターン毎", " turns");
        return ss.str();
    case RandomArtActType::TERROR:
        return _("3*(レベル+10) ターン毎", "every 3 * (level+10) turns");
    case RandomArtActType::MURAMASA:
        return _("確率50%で壊れる", "(destroyed 50%)");
    default:
        return "undefined";
    }
}

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/汎用）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static std::string item_activation_aux(const ItemEntity *o_ptr)
{
    const auto it = o_ptr->find_activation_info();
    if (it == activation_info.end()) {
        return _("未定義", "something undefined");
    }

    const auto desc = build_activation_description(*it, *o_ptr);
    const auto timeout = build_timeout_description(*it, *o_ptr);
    std::stringstream ss;
    ss << desc << _(" : ", " ") << timeout;
    return ss.str();
}

/*!
 * @brief オブジェクトの発動効果名称を返す（メインルーチン） /
 * Determine the "Activation" (if any) for an artifact Return a string, or nullptr for "no activation"
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
std::string activation_explanation(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    if (flags.has_not(TR_ACTIVATE)) {
        return _("なし", "nothing");
    }

    if (o_ptr->has_activation()) {
        return item_activation_aux(o_ptr);
    }

    const auto tval = o_ptr->bi_key.tval();
    if (tval == ItemKindType::WHISTLE) {
        return _("ペット呼び寄せ : 100+d100ターン毎", "call pet every 100+d100 turns");
    }

    if (tval == ItemKindType::CAPTURE) {
        return _("モンスターを捕える、又は解放する。", "captures or releases a monster.");
    }

    return _("何も起きない", "Nothing");
}

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
