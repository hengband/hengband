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
#include "artifact/artifact-info.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "object-enchant/activation-info-table.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object-enchant/object-ego.h"
#include "object/object-flags.h"
#include "object/object-kind.h"
#include "player/player-realm.h"
#include "realm/realm-names-table.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/ブレス）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_dragon_breath(object_type *o_ptr)
{
    static char desc[256];
    int n = 0;

    auto flgs = object_flags(o_ptr);
    strcpy(desc, _("", "breathe "));

    for (int i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (flgs.has(dragonbreath_info[i].flag)) {
            if (n > 0)
                strcat(desc, _("、", ", "));

            strcat(desc, dragonbreath_info[i].name);
            n++;
        }
    }

    strcat(desc, _("のブレス(250)", " (250)"));
    return (desc);
}

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/汎用）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_aux(object_type *o_ptr)
{
    static char activation_detail[512];
    char timeout[64];
    auto tmp_act_ptr = find_activation_info(o_ptr);
    if (!tmp_act_ptr.has_value()) {
        return _("未定義", "something undefined");
    }

    auto *act_ptr = tmp_act_ptr.value();
    concptr desc = act_ptr->desc;
    switch (act_ptr->index) {
    case RandomArtActType::NONE:
        break;
    case RandomArtActType::BR_FIRE:
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
            desc = _("火炎のブレス (200) と火への耐性", "breathe fire (200) and resist fire");
        break;
    case RandomArtActType::BR_COLD:
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
            desc = _("冷気のブレス (200) と冷気への耐性", "breathe cold (200) and resist cold");
        break;
    case RandomArtActType::BR_DRAGON:
        desc = item_activation_dragon_breath(o_ptr);
        break;
    case RandomArtActType::AGGRAVATE:
        if (o_ptr->name1 == ART_HYOUSIGI)
            desc = _("拍子木を打ちならす", "beat wooden clappers");
        break;
    case RandomArtActType::ACID_BALL_AND_RESISTANCE:
        desc = _("アシッド・ボール (100) と酸への耐性", "ball of acid (100) and resist acid");
        break;
    case RandomArtActType::FIRE_BALL_AND_RESISTANCE:
        desc = _("ファイア・ボール (100) と火への耐性", "ball of fire (100) and resist fire");
        break;
    case RandomArtActType::COLD_BALL_AND_RESISTANCE:
        desc = _("アイス・ボール (100) と冷気への耐性", "ball of cold (100) and resist cold");
        break;
    case RandomArtActType::ELEC_BALL_AND_RESISTANCE:
        desc = _("サンダー・ボール (100) と電撃への耐性", "ball of elec (100) and resist elec");
        break;
    case RandomArtActType::POIS_BALL_AND_RESISTANCE:
        desc = _("ポイズン・ボール (100) と毒への耐性", "ball of poison (100) and resist elec");
        break;
    case RandomArtActType::RESIST_ACID:
        desc = _("一時的な酸への耐性", "temporary resist acid");
        break;
    case RandomArtActType::RESIST_FIRE:
        desc = _("一時的な火への耐性", "temporary resist fire");
        break;
    case RandomArtActType::RESIST_COLD:
        desc = _("一時的な冷気への耐性", "temporary resist cold");
        break;
    case RandomArtActType::RESIST_ELEC:
        desc = _("一時的な電撃への耐性", "temporary resist elec");
        break;
    case RandomArtActType::RESIST_POIS:
        desc = _("一時的な毒への耐性", "temporary resist elec");
        break;
    default:
        break;
    }

    /* Timeout description */
    int constant = act_ptr->timeout.constant;
    int dice = act_ptr->timeout.dice;
    if (constant == 0 && dice == 0) {
        /* We can activate it every turn */
        strcpy(timeout, _("いつでも", "every turn"));
    } else if (constant < 0) {
        /* Activations that have special timeout */
        switch (act_ptr->index) {
        case RandomArtActType::BR_FIRE:
            sprintf(timeout, _("%d ターン毎", "every %d turns"), ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250);
            break;
        case RandomArtActType::BR_COLD:
            sprintf(timeout, _("%d ターン毎", "every %d turns"), ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250);
            break;
        case RandomArtActType::TERROR:
            strcpy(timeout, _("3*(レベル+10) ターン毎", "every 3 * (level+10) turns"));
            break;
        case RandomArtActType::MURAMASA:
            strcpy(timeout, _("確率50%で壊れる", "(destroyed 50%)"));
            break;
        default:
            strcpy(timeout, "undefined");
            break;
        }
    } else {
        char constant_str[16], dice_str[16];
        sprintf(constant_str, "%d", constant);
        sprintf(dice_str, "d%d", dice);
        sprintf(timeout, _("%s%s%s ターン毎", "every %s%s%s turns"), (constant > 0) ? constant_str : "", (constant > 0 && dice > 0) ? "+" : "",
            (dice > 0) ? dice_str : "");
    }

    sprintf(activation_detail, _("%s : %s", "%s %s"), desc, timeout);
    return activation_detail;
}

/*!
 * @brief オブジェクトの発動効果名称を返す（メインルーチン） /
 * Determine the "Activation" (if any) for an artifact Return a string, or nullptr for "no activation"
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
concptr activation_explanation(object_type *o_ptr)
{
    auto flgs = object_flags(o_ptr);
    if (flgs.has_not(TR_ACTIVATE))
        return (_("なし", "nothing"));

    if (activation_index(o_ptr) > RandomArtActType::NONE) {
        return item_activation_aux(o_ptr);
    }

    if (o_ptr->tval == TV_WHISTLE) {
        return _("ペット呼び寄せ : 100+d100ターン毎", "call pet every 100+d100 turns");
    }

    if (o_ptr->tval == TV_CAPTURE) {
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
char index_to_label(int i) { return (i < INVEN_MAIN_HAND) ? (I2A(i)) : (I2A(i - INVEN_MAIN_HAND)); }

/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
int16_t wield_slot(player_type *player_ptr, const object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD: {
        if (!player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx)
            return (INVEN_MAIN_HAND);
        if (player_ptr->inventory_list[INVEN_SUB_HAND].k_idx)
            return (INVEN_MAIN_HAND);
        return (INVEN_SUB_HAND);
    }
    case TV_CAPTURE:
    case TV_CARD:
    case TV_SHIELD: {
        if (!player_ptr->inventory_list[INVEN_SUB_HAND].k_idx)
            return (INVEN_SUB_HAND);
        if (player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx)
            return (INVEN_SUB_HAND);
        return (INVEN_MAIN_HAND);
    }
    case TV_BOW: {
        return (INVEN_BOW);
    }
    case TV_RING: {
        if (!player_ptr->inventory_list[INVEN_MAIN_RING].k_idx)
            return (INVEN_MAIN_RING);

        return (INVEN_SUB_RING);
    }
    case TV_AMULET:
    case TV_WHISTLE: {
        return (INVEN_NECK);
    }
    case TV_LITE: {
        return (INVEN_LITE);
    }
    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR: {
        return (INVEN_BODY);
    }
    case TV_CLOAK: {
        return (INVEN_OUTER);
    }
    case TV_CROWN:
    case TV_HELM: {
        return (INVEN_HEAD);
    }
    case TV_GLOVES: {
        return (INVEN_ARMS);
    }
    case TV_BOOTS: {
        return (INVEN_FEET);
    }

    default:
        break;
    }

    return -1;
}

/*!
 * @brief tval/sval指定のベースアイテムがプレイヤーの使用可能な魔法書かどうかを返す /
 * Hack: Check if a spellbook is one of the realms we can use. -- TY
 * @param book_tval ベースアイテムのtval
 * @param book_sval ベースアイテムのsval
 * @return 使用可能な魔法書ならばTRUEを返す。
 */
bool check_book_realm(player_type *player_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval)
{
    if (book_tval < TV_LIFE_BOOK)
        return false;
    if (player_ptr->pclass == CLASS_SORCERER) {
        return is_magic(tval2realm(book_tval));
    } else if (player_ptr->pclass == CLASS_RED_MAGE) {
        if (is_magic(tval2realm(book_tval)))
            return ((book_tval == TV_ARCANE_BOOK) || (book_sval < 2));
    }

    return (get_realm1_book(player_ptr) == book_tval || get_realm2_book(player_ptr) == book_tval);
}

object_type *ref_item(player_type *player_ptr, INVENTORY_IDX item)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    return item >= 0 ? &player_ptr->inventory_list[item] : &(floor_ptr->o_list[0 - item]);
}

/*
 * Return the "attr" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
TERM_COLOR object_attr(object_type *o_ptr)
{
    return ((k_info[o_ptr->k_idx].flavor)
            ? (k_info[k_info[o_ptr->k_idx].flavor].x_attr)
            : ((!o_ptr->k_idx || (o_ptr->tval != TV_CORPSE) || (o_ptr->sval != SV_CORPSE) || (k_info[o_ptr->k_idx].x_attr != TERM_DARK))
                    ? (k_info[o_ptr->k_idx].x_attr)
                    : (r_info[o_ptr->pval].x_attr)));
}
