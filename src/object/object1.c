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

#include "object/object1.h"
#include "cmd-item/cmd-activate.h"
#include "cmd-item/cmd-smith.h"
#include "combat/snipe.h"
#include "floor/floor.h"
#include "inventory/player-inventory.h"
#include "io/files-util.h"
#include "io/read-pref-file.h"
#include "monster/monster.h"
#include "object-enchant/artifact.h" // 相互参照している.
#include "object-enchant/item-apply-magic.h"
#include "perception/object-perception.h"
#include "object-enchant/object-ego.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object-enchant/special-object-flags.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "player/player-class.h"
#include "player/player-move.h"
#include "system/system-variables.h"
#include "term/term-color-types.h"
#include "util/util.h"
#include "view/display-main-window.h"

/*!
 * @brief オブジェクトのフラグ類を配列に与える
 * Obtain the "flags" for an item
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 * @return なし
 */
void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Base object */
	for (int i = 0; i < TR_FLAG_SIZE; i++)
	{
		flgs[i] = k_ptr->flags[i];
	}

	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];
		for (int i = 0; i < TR_FLAG_SIZE; i++)
		{
			flgs[i] = a_ptr->flags[i];
		}
	}

	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		for (int i = 0; i < TR_FLAG_SIZE; i++)
		{
			flgs[i] |= e_ptr->flags[i];
		}

		if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_SH_FIRE);
		}
		else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_INFRA);
		}
		else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_RES_BLIND);
			remove_flag(flgs, TR_SEE_INVIS);
		}
	}

	/* Random artifact ! */
	for (int i = 0; i < TR_FLAG_SIZE; i++)
	{
		flgs[i] |= o_ptr->art_flags[i];
	}

	if (object_is_smith(o_ptr))
	{
		int add = o_ptr->xtra3 - 1;
		if (add < TR_FLAG_MAX)
		{
			add_flag(flgs, add);
		}
		else if (add == ESSENCE_TMP_RES_ACID)
		{
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_SH_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_SH_FIRE);
		}
		else if (add == ESSENCE_SH_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_SH_ELEC);
		}
		else if (add == ESSENCE_SH_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SH_COLD);
		}
		else if (add == ESSENCE_RESISTANCE)
		{
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_RES_COLD);
		}
		else if (add == TR_IMPACT)
		{
			add_flag(flgs, TR_ACTIVATE);
		}
	}
}


/*!
 * @brief オブジェクトの明示されているフラグ類を取得する
 * Obtain the "flags" for an item which are known to the player
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 * @return なし
 */
void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	bool spoil = FALSE;
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	for (int i = 0; i < TR_FLAG_SIZE; i++)
	{
		flgs[i] = 0;
	}

	if (!object_is_aware(o_ptr)) return;

	/* Base object */
	for (int i = 0; i < TR_FLAG_SIZE; i++)
	{
		flgs[i] = k_ptr->flags[i];
	}

	if (!object_is_known(o_ptr)) return;

	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];
		for (int i = 0; i < TR_FLAG_SIZE; i++)
		{
			flgs[i] |= e_ptr->flags[i];
		}

		if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_SH_FIRE);
		}
		else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_INFRA);
		}
		else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_RES_BLIND);
			remove_flag(flgs, TR_SEE_INVIS);
		}
	}

	if (spoil || object_is_fully_known(o_ptr))
	{
		if (object_is_fixed_artifact(o_ptr))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			for (int i = 0; i < TR_FLAG_SIZE; i++)
			{
				flgs[i] = a_ptr->flags[i];
			}
		}

		/* Random artifact ! */
		for (int i = 0; i < TR_FLAG_SIZE; i++)
		{
			flgs[i] |= o_ptr->art_flags[i];
		}
	}

	if (!object_is_smith(o_ptr)) return;

	int add = o_ptr->xtra3 - 1;
	if (add < TR_FLAG_MAX)
	{
		add_flag(flgs, add);
	}
	else if (add == ESSENCE_TMP_RES_ACID)
	{
		add_flag(flgs, TR_RES_ACID);
	}
	else if (add == ESSENCE_TMP_RES_ELEC)
	{
		add_flag(flgs, TR_RES_ELEC);
	}
	else if (add == ESSENCE_TMP_RES_FIRE)
	{
		add_flag(flgs, TR_RES_FIRE);
	}
	else if (add == ESSENCE_TMP_RES_COLD)
	{
		add_flag(flgs, TR_RES_COLD);
	}
	else if (add == ESSENCE_SH_FIRE)
	{
		add_flag(flgs, TR_RES_FIRE);
		add_flag(flgs, TR_SH_FIRE);
	}
	else if (add == ESSENCE_SH_ELEC)
	{
		add_flag(flgs, TR_RES_ELEC);
		add_flag(flgs, TR_SH_ELEC);
	}
	else if (add == ESSENCE_SH_COLD)
	{
		add_flag(flgs, TR_RES_COLD);
		add_flag(flgs, TR_SH_COLD);
	}
	else if (add == ESSENCE_RESISTANCE)
	{
		add_flag(flgs, TR_RES_ACID);
		add_flag(flgs, TR_RES_ELEC);
		add_flag(flgs, TR_RES_FIRE);
		add_flag(flgs, TR_RES_COLD);
	}
}


/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/ブレス）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_dragon_breath(object_type *o_ptr)
{
	static char desc[256];
	BIT_FLAGS flgs[TR_FLAG_SIZE]; /* for resistance flags */
	int n = 0;

	object_flags(o_ptr, flgs);
	strcpy(desc, _("", "breath "));

	for (int i = 0; dragonbreath_info[i].flag != 0; i++)
	{
		if (have_flag(flgs, dragonbreath_info[i].flag))
		{
			if (n > 0) strcat(desc, _("、", ", "));

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
static concptr item_activation_aux(object_type *o_ptr)
{
	static char activation_detail[256];
	char timeout[32];
	const activation_type* const act_ptr = find_activation_info(o_ptr);

	if (!act_ptr) return _("未定義", "something undefined");

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
		desc = item_activation_dragon_breath(o_ptr);
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
			sprintf(timeout, _("%d ターン毎", "every %d turns"),
				((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250);
			break;
		case ACT_BR_COLD:
			sprintf(timeout, _("%d ターン毎", "every %d turns"),
				((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250);
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
		sprintf(timeout, _("%s%s%s ターン毎", "every %s%s%s turns"),
			(constant > 0) ? constant_str : "",
			(constant > 0 && dice > 0) ? "+" : "",
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
concptr item_activation(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);
	if (!(have_flag(flgs, TR_ACTIVATE))) return (_("なし", "nothing"));

	if (activation_index(o_ptr))
	{
		return item_activation_aux(o_ptr);
	}

	if (o_ptr->tval == TV_WHISTLE)
	{
		return _("ペット呼び寄せ : 100+d100ターン毎", "call pet every 100+d100 turns");
	}

	if (o_ptr->tval == TV_CAPTURE)
	{
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
	return (i < INVEN_RARM) ? (I2A(i)) : (I2A(i - INVEN_RARM));
}


/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
s16b wield_slot(player_type *owner_ptr, object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			if (!owner_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_RARM);
			if (owner_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_RARM);
			return (INVEN_LARM);
		}
		case TV_CAPTURE:
		case TV_CARD:
		case TV_SHIELD:
		{
			if (!owner_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_LARM);
			if (owner_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_LARM);
			return (INVEN_RARM);
		}
		case TV_BOW:
		{
			return (INVEN_BOW);
		}
		case TV_RING:
		{
			if (!owner_ptr->inventory_list[INVEN_RIGHT].k_idx) return (INVEN_RIGHT);

			return (INVEN_LEFT);
		}
		case TV_AMULET:
		case TV_WHISTLE:
		{
			return (INVEN_NECK);
		}
		case TV_LITE:
		{
			return (INVEN_LITE);
		}
		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		{
			return (INVEN_BODY);
		}
		case TV_CLOAK:
		{
			return (INVEN_OUTER);
		}
		case TV_CROWN:
		case TV_HELM:
		{
			return (INVEN_HEAD);
		}
		case TV_GLOVES:
		{
			return (INVEN_HANDS);
		}
		case TV_BOOTS:
		{
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
	if (book_tval < TV_LIFE_BOOK) return FALSE;
	if (owner_ptr->pclass == CLASS_SORCERER)
	{
		return is_magic(tval2realm(book_tval));
	}
	else if (owner_ptr->pclass == CLASS_RED_MAGE)
	{
		if (is_magic(tval2realm(book_tval)))
			return ((book_tval == TV_ARCANE_BOOK) || (book_sval < 2));
	}

	return (REALM1_BOOK == book_tval || REALM2_BOOK == book_tval);
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
    return((k_info[o_ptr->k_idx].flavor) ? (k_info[k_info[o_ptr->k_idx].flavor].x_attr)
                                   : ((!o_ptr->k_idx || (o_ptr->tval != TV_CORPSE) || (o_ptr->sval != SV_CORPSE) || (k_info[o_ptr->k_idx].x_attr != TERM_DARK))
                                           ? (k_info[o_ptr->k_idx].x_attr)
                                           : (r_info[o_ptr->pval].x_attr)));
}
