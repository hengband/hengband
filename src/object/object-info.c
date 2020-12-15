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
#include "artifact/artifact-info.h"
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
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/ブレス）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_dragon_breath(player_type *owner_ptr, object_type *o_ptr)
{
    static char desc[256];
    BIT_FLAGS flgs[TR_FLAG_SIZE]; /* for resistance flags */
    int n = 0;

    object_flags(owner_ptr, o_ptr, flgs);
    strcpy(desc, _("", "breath "));

    for (int i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (has_flag(flgs, dragonbreath_info[i].flag)) {
            if (n > 0)
                strcat(desc, _("、", ", "));

            strcat(desc, dragonbreath_info[i].name);
            n++;
        }
    }

    strcat(desc, _("のブレス(250)", ""));
    return (desc);
}

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/汎用）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_aux(player_type *owner_ptr, object_type *o_ptr)
{
    static char activation_detail[256];
    char timeout[32];
    const activation_type *const act_ptr = find_activation_info(owner_ptr, o_ptr);

    if (!act_ptr)
        return _("未定義", "something undefined");

    concptr desc = act_ptr->desc;
    switch (act_ptr->index) {
    case ACT_BR_FIRE:
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
            desc = _("火炎のブレス (200) と火への耐性", "breath of fire (200) and resist fire");
        break;
    case ACT_BR_COLD:
        if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
            desc = _("冷気のブレス (200) と冷気への耐性", "breath of cold (200) and resist cold");
        break;
    case ACT_BR_DRAGON:
        desc = item_activation_dragon_breath(owner_ptr, o_ptr);
        break;
    case ACT_AGGRAVATE:
        if (o_ptr->name1 == ART_HYOUSIGI)
            desc = _("拍子木を打ちならす", "beat wooden clappers");
        break;
    case ACT_RESIST_ACID:
        if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ACID)) || (o_ptr->name2 == EGO_BRAND_ACID))
            desc = _("アシッド・ボール (100) と酸への耐性", "ball of acid (100) and resist acid");
        break;
    case ACT_RESIST_FIRE:
        if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) || (o_ptr->name2 == EGO_BRAND_FIRE))
            desc = _("ファイア・ボール (100) と火への耐性", "ball of fire (100) and resist fire");
        break;
    case ACT_RESIST_COLD:
        if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) || (o_ptr->name2 == EGO_BRAND_COLD))
            desc = _("アイス・ボール (100) と冷気への耐性", "ball of cold (100) and resist cold");
        break;
    case ACT_RESIST_ELEC:
        if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ELEC)) || (o_ptr->name2 == EGO_BRAND_ELEC))
            desc = _("サンダー・ボール (100) と電撃への耐性", "ball of elec (100) and resist elec");
        break;
    case ACT_RESIST_POIS:
        if (o_ptr->name2 == EGO_BRAND_POIS)
            desc = _("悪臭雲 (100) と毒への耐性", "ball of poison (100) and resist elec");
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
        case ACT_BR_FIRE:
            sprintf(timeout, _("%d ターン毎", "every %d turns"), ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250);
            break;
        case ACT_BR_COLD:
            sprintf(timeout, _("%d ターン毎", "every %d turns"), ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250);
            break;
        case ACT_TERROR:
            strcpy(timeout, _("3*(レベル+10) ターン毎", "every 3 * (level+10) turns"));
            break;
        case ACT_MURAMASA:
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
 * Determine the "Activation" (if any) for an artifact Return a string, or NULL for "no activation"
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
concptr activation_explanation(player_type *owner_ptr, object_type *o_ptr)
{
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(owner_ptr, o_ptr, flgs);
    if (!(has_flag(flgs, TR_ACTIVATE)))
        return (_("なし", "nothing"));

    if (activation_index(owner_ptr, o_ptr)) {
        return item_activation_aux(owner_ptr, o_ptr);
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
char index_to_label(int i) { return (i < INVEN_RARM) ? (I2A(i)) : (I2A(i - INVEN_RARM)); }

/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
s16b wield_slot(player_type *owner_ptr, object_type *o_ptr)
{
    switch (o_ptr->tval) {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD: {
        if (!owner_ptr->inventory_list[INVEN_RARM].k_idx)
            return (INVEN_RARM);
        if (owner_ptr->inventory_list[INVEN_LARM].k_idx)
            return (INVEN_RARM);
        return (INVEN_LARM);
    }
    case TV_CAPTURE:
    case TV_CARD:
    case TV_SHIELD: {
        if (!owner_ptr->inventory_list[INVEN_LARM].k_idx)
            return (INVEN_LARM);
        if (owner_ptr->inventory_list[INVEN_RARM].k_idx)
            return (INVEN_LARM);
        return (INVEN_RARM);
    }
    case TV_BOW: {
        return (INVEN_BOW);
    }
    case TV_RING: {
        if (!owner_ptr->inventory_list[INVEN_RIGHT].k_idx)
            return (INVEN_RIGHT);

        return (INVEN_LEFT);
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
        return (INVEN_HANDS);
    }
    case TV_BOOTS: {
        return (INVEN_FEET);
    }
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
bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval)
{
    if (book_tval < TV_LIFE_BOOK)
        return FALSE;
    if (owner_ptr->pclass == CLASS_SORCERER) {
        return is_magic(tval2realm(book_tval));
    } else if (owner_ptr->pclass == CLASS_RED_MAGE) {
        if (is_magic(tval2realm(book_tval)))
            return ((book_tval == TV_ARCANE_BOOK) || (book_sval < 2));
    }

    return (get_realm1_book(owner_ptr) == book_tval || get_realm2_book(owner_ptr) == book_tval);
}

object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    return item >= 0 ? &owner_ptr->inventory_list[item] : &(floor_ptr->o_list[0 - item]);
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
