/*!
 * @file obj_kind.c
 * @brief ベースアイテムの実装
 * @date 2014/01/10
 * @author
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"

/*!
 * @brief オブジェクトが薬であるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool object_is_potion(object_type *o_ptr)
{
	return (k_info[o_ptr->k_idx].tval == TV_POTION);
}


/*!
 * @brief オブジェクトが賞金首の報酬対象になるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが報酬対象になるならTRUEを返す
 */
bool object_is_shoukinkubi(object_type *o_ptr)
{
	int i;

	/* Require corpse or skeleton */
	if (o_ptr->tval != TV_CORPSE) return FALSE;

	/* No wanted monsters in vanilla town */
	if (vanilla_town) return FALSE;

	/* Today's wanted */
	if (p_ptr->today_mon > 0 && (streq(r_name + r_info[o_ptr->pval].name, r_name + r_info[today_mon].name))) return TRUE;

	/* Tsuchinoko */
	if (o_ptr->pval == MON_TSUCHINOKO) return TRUE;

	/* Unique monster */
	for (i = 0; i < MAX_KUBI; i++)
		if (o_ptr->pval == kubi_r_idx[i]) break;
	if (i < MAX_KUBI) return TRUE;

	/* Not wanted */
	return FALSE;
}

/*!
 * @brief オブジェクトがプレイヤーの職業に応じた適正武器か否かを返す / Favorite weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが適正武器ならばTRUEを返す
 */
bool object_is_favorite(object_type *o_ptr)
{
	/* Only melee weapons match */
	if (!(o_ptr->tval == TV_POLEARM ||
	      o_ptr->tval == TV_SWORD ||
	      o_ptr->tval == TV_DIGGING ||
	      o_ptr->tval == TV_HAFTED))
	{
		return FALSE;
	}

	/* Favorite weapons are varied depend on the class */
	switch (p_ptr->pclass)
	{
	case CLASS_PRIEST:
	{
		u32b flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		if (!have_flag(flgs, TR_BLESSED) && 
		    !(o_ptr->tval == TV_HAFTED))
			return FALSE;
		break;
	}

	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		/* Icky to wield? */
		if (!(s_info[p_ptr->pclass].w_max[o_ptr->tval-TV_WEAPON_BEGIN][o_ptr->sval]))
			return FALSE;
		break;

	case CLASS_BEASTMASTER:
	case CLASS_CAVALRY:
	{
		u32b flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		/* Is it known to be suitable to using while riding? */
		if (!(have_flag(flgs, TR_RIDING)))
			return FALSE;

		break;
	}

	case CLASS_NINJA:
		/* Icky to wield? */
		if (s_info[p_ptr->pclass].w_max[o_ptr->tval-TV_WEAPON_BEGIN][o_ptr->sval] <= WEAPON_EXP_BEGINNER)
			return FALSE;
		break;

	default:
		/* All weapons are okay for non-special classes */
		return TRUE;
	}

	return TRUE;
}


/*!
 * @brief オブジェクトがレアアイテムかどうかを返す /
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return レアアイテムならばTRUEを返す
 */
bool object_is_rare(object_type *o_ptr)
{
	switch(o_ptr->tval)
	{
	case TV_HAFTED:
		if (o_ptr->sval == SV_MACE_OF_DISRUPTION ||
		    o_ptr->sval == SV_WIZSTAFF) return TRUE;
		break;

	case TV_POLEARM:
		if (o_ptr->sval == SV_SCYTHE_OF_SLICING ||
		    o_ptr->sval == SV_DEATH_SCYTHE) return TRUE;
		break;

	case TV_SWORD:
		if (o_ptr->sval == SV_BLADE_OF_CHAOS ||
		    o_ptr->sval == SV_DIAMOND_EDGE ||
		    o_ptr->sval == SV_DOKUBARI ||
		    o_ptr->sval == SV_HAYABUSA) return TRUE;
		break;

	case TV_SHIELD:
		if (o_ptr->sval == SV_DRAGON_SHIELD ||
		    o_ptr->sval == SV_MIRROR_SHIELD) return TRUE;
		break;

	case TV_HELM:
		if (o_ptr->sval == SV_DRAGON_HELM) return TRUE;
		break;

	case TV_BOOTS:
		if (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) return TRUE;
		break;

	case TV_CLOAK:
		if (o_ptr->sval == SV_ELVEN_CLOAK ||
		    o_ptr->sval == SV_ETHEREAL_CLOAK ||
		    o_ptr->sval == SV_SHADOW_CLOAK) return TRUE;
		break;

	case TV_GLOVES:
		if (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES) return TRUE;
		break;

	case TV_SOFT_ARMOR:
		if (o_ptr->sval == SV_KUROSHOUZOKU ||
		    o_ptr->sval == SV_ABUNAI_MIZUGI) return TRUE;
		break;

	case TV_DRAG_ARMOR:
		return TRUE;

	default:
		break;
	}

	/* Any others are not "rare" objects. */
	return FALSE;
}


/*!
 * @brief オブジェクトが武器として装備できるかどうかを返す / Check if an object is weapon (including bows and ammo)
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器として使えるならばTRUEを返す
 */
bool object_is_weapon(object_type *o_ptr)
{
	if (TV_WEAPON_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEAPON_END) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが武器や矢弾として使用できるかを返す / Check if an object is weapon (including bows and ammo)
 * Rare weapons/aromors including Blade of Chaos, Dragon armors, etc.
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器や矢弾として使えるならばTRUEを返す
 */
bool object_is_weapon_ammo(object_type *o_ptr)
{
	if (TV_MISSILE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEAPON_END) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトが矢弾として使用できるかどうかを返す / Check if an object is ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 矢弾として使えるならばTRUEを返す
 */
bool object_is_ammo(object_type *o_ptr)
{
	if (TV_MISSILE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_MISSILE_END) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトが防具として装備できるかどうかを返す / Check if an object is armour
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 矢弾として使えるならばTRUEを返す
 */
bool object_is_armour(object_type *o_ptr)
{
	if (TV_ARMOR_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_ARMOR_END) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトが武器、防具、矢弾として使用できるかを返す / Check if an object is weapon, armour or ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 武器、防具、矢弾として使えるならばTRUEを返す
 */
bool object_is_weapon_armour_ammo(object_type *o_ptr)
{
	if (object_is_weapon_ammo(o_ptr) || object_is_armour(o_ptr)) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが近接武器として装備できるかを返す / Melee weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 近接武器として使えるならばTRUEを返す
 */
bool object_is_melee_weapon(object_type *o_ptr)
{
	if (TV_DIGGING <= o_ptr->tval && o_ptr->tval <= TV_SWORD) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが装備可能であるかを返す / Wearable including all weapon, all armour, bow, light source, amulet, and ring
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 装備可能ならばTRUEを返す
 */
bool object_is_wearable(object_type *o_ptr)
{
	if (TV_WEARABLE_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_WEARABLE_END) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが装備品であるかを返す(object_is_wearableに矢弾を含む) / Equipment including all wearable objects and ammo
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 装備品ならばTRUEを返す
 */
bool object_is_equipment(object_type *o_ptr)
{
	if (TV_EQUIP_BEGIN <= o_ptr->tval && o_ptr->tval <= TV_EQUIP_END) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが強化不能武器であるかを返す / Poison needle can not be enchanted
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化不能ならばTRUEを返す
 */
bool object_refuse_enchant_weapon(object_type *o_ptr)
{
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_DOKUBARI) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが強化可能武器であるかを返す /
 * Check if an object is weapon (including bows and ammo) and allows enchantment
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化可能ならばTRUEを返す
 */
bool object_allow_enchant_weapon(object_type *o_ptr)
{
	if (object_is_weapon_ammo(o_ptr) && !object_refuse_enchant_weapon(o_ptr)) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが強化可能な近接武器であるかを返す /
 * Check if an object is melee weapon and allows enchantment
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 強化可能な近接武器ならばTRUEを返す
 */
bool object_allow_enchant_melee_weapon(object_type *o_ptr)
{
	if (object_is_melee_weapon(o_ptr) && !object_refuse_enchant_weapon(o_ptr)) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが鍛冶師のエッセンス付加済みかを返す /
 * Check if an object is made by a smith's special ability
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return エッセンス付加済みならばTRUEを返す
 */
bool object_is_smith(object_type *o_ptr)
{
	if (object_is_weapon_armour_ammo(o_ptr) && o_ptr->xtra3) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトがアーティファクトかを返す /
 * Check if an object is artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return アーティファクトならばTRUEを返す
 */
bool object_is_artifact(object_type *o_ptr)
{
	if (object_is_fixed_artifact(o_ptr) || o_ptr->art_name) return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトがランダムアーティファクトかを返す /
 * Check if an object is random artifact
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return ランダムアーティファクトならばTRUEを返す
 */
bool object_is_random_artifact(object_type *o_ptr)
{
	if (object_is_artifact(o_ptr) && !object_is_fixed_artifact(o_ptr)) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトが通常のアイテム(アーティファクト、エゴ、鍛冶師エッセンス付加いずれでもない)かを返す /
 * Check if an object is neither artifact, ego, nor 'smith' object
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 通常のアイテムならばTRUEを返す
 */
bool object_is_nameless(object_type *o_ptr)
{
	if (!object_is_artifact(o_ptr) && !object_is_ego(o_ptr) && !object_is_smith(o_ptr))
		return TRUE;

	return FALSE;
}


/*!
 * @brief オブジェクトが両手持ち可能な武器かを返す /
 * Check if an object is melee weapon and allows wielding with two-hands
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 両手持ち可能ならばTRUEを返す
 */
bool object_allow_two_hands_wielding(object_type *o_ptr)
{
	if (object_is_melee_weapon(o_ptr) && ((o_ptr->weight > 99) || (o_ptr->tval == TV_POLEARM))) return TRUE;

	return FALSE;
}
