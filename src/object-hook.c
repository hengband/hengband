
#include "angband.h"
#include "object-hook.h"

/*!
* @brief 対象のアイテムが矢やクロスボウの矢の材料になるかを返す。/
* Hook to determine if an object is contertible in an arrow/bolt
* @param o_ptr オブジェクトの構造体の参照ポインタ。
* @return 材料にできるならTRUEを返す
*/
bool item_tester_hook_convertible(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_JUNK) || (o_ptr->tval == TV_SKELETON)) return TRUE;

	if ((o_ptr->tval == TV_CORPSE) && (o_ptr->sval == SV_SKELETON)) return TRUE;
	/* Assume not */
	return (FALSE);
}

/*!
* @brief 武器匠の「武器」鑑定対象になるかを判定する。/ Hook to specify "weapon"
* @param o_ptr オブジェクトの構造体の参照ポインタ。
* @return 対象になるならTRUEを返す。
*/
bool item_tester_hook_orthodox_melee_weapons(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	{
		return (TRUE);
	}
	case TV_SWORD:
	{
		if (o_ptr->sval != SV_DOKUBARI) return (TRUE);
	}
	}

	return (FALSE);
}

/*!
* @brief オブジェクトが右手か左手に装備できる武器かどうかの判定
* @param o_ptr 判定するオブジェクトの構造体参照ポインタ
* @return 右手か左手の武器として装備できるならばTRUEを返す。
*/
bool item_tester_hook_melee_weapon(object_type *o_ptr)
{
	/* Check for a usable slot */
	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


/*!
* @brief 武器匠の「矢弾」鑑定対象になるかを判定する。/ Hook to specify "weapon"
* @param o_ptr オブジェクトの構造体の参照ポインタ。
* @return 対象になるならTRUEを返す。
*/
bool item_tester_hook_ammo(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	{
		return (TRUE);
	}
	}

	return (FALSE);
}

/*!
* @brief 修復対象となる壊れた武器かを判定する。 / Hook to specify "broken weapon"
* @param o_ptr オブジェクトの構造体の参照ポインタ。
* @return 修復対象になるならTRUEを返す。
*/
bool item_tester_hook_broken_weapon(object_type *o_ptr)
{
	if (o_ptr->tval != TV_SWORD) return FALSE;

	switch (o_ptr->sval)
	{
	case SV_BROKEN_DAGGER:
	case SV_BROKEN_SWORD:
		return TRUE;
	}

	return FALSE;
}

/*!
* @brief オブジェクトが投射可能な武器かどうかを返す。
* @param o_ptr 判定するオブジェクトの構造体参照ポインタ
* @return 投射可能な武器ならばTRUE
*/
bool item_tester_hook_boomerang(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_HAFTED)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*!
* @brief オブジェクトをプレイヤーが食べることができるかを判定する /
* Hook to determine if an object is eatable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 食べることが可能ならばTRUEを返す
*/
bool item_tester_hook_eatable(object_type *o_ptr)
{
	if (o_ptr->tval == TV_FOOD) return TRUE;

#if 0
	if (prace_is_(RACE_SKELETON))
	{
		if (o_ptr->tval == TV_SKELETON ||
			(o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
			return TRUE;
	}
	else
#endif

		if (prace_is_(RACE_SKELETON) ||
			prace_is_(RACE_GOLEM) ||
			prace_is_(RACE_ZOMBIE) ||
			prace_is_(RACE_SPECTRE))
		{
			if (o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND)
				return TRUE;
		}
		else if (prace_is_(RACE_DEMON) ||
			(mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON))
		{
			if (o_ptr->tval == TV_CORPSE &&
				o_ptr->sval == SV_CORPSE &&
				my_strchr("pht", r_info[o_ptr->pval].d_char))
				return TRUE;
		}

	/* Assume not */
	return (FALSE);
}

/*!
* @brief オブジェクトがどちらの手にも装備できる武器かどうかの判定
* @param o_ptr 判定するオブジェクトの構造体参照ポインタ
* @return 左右両方の手で装備できるならばTRUEを返す。
*/
bool item_tester_hook_mochikae(object_type *o_ptr)
{
	/* Check for a usable slot */
	if (((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) ||
		(o_ptr->tval == TV_SHIELD) || (o_ptr->tval == TV_CAPTURE) ||
		(o_ptr->tval == TV_CARD)) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}

/*!
* @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
* Hook to determine if an object is activatable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 魔道具として発動可能ならばTRUEを返す
*/
bool item_tester_hook_activate(object_type *o_ptr)
{
	u32b flgs[TR_FLAG_SIZE];

	/* Not known */
	if (!object_is_known(o_ptr)) return (FALSE);

	/* Extract the flags */
	object_flags(o_ptr, flgs);

	/* Check activation flag */
	if (have_flag(flgs, TR_ACTIVATE)) return (TRUE);

	/* Assume not */
	return (FALSE);
}

/*!
* @brief オブジェクトを防具として装備できるかの判定 / The "wearable" tester
* @param o_ptr 判定するオブジェクトの構造体参照ポインタ
* @return オブジェクトが防具として装備できるならTRUEを返す。
*/
bool item_tester_hook_wear(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI))
		if (p_ptr->psex == SEX_MALE) return FALSE;

	/* Check for a usable slot */
	if (wield_slot(o_ptr) >= INVEN_RARM) return (TRUE);

	/* Assume not wearable */
	return (FALSE);
}


/*!
* @brief オブジェクトをプレイヤーが簡易使用コマンドで利用できるかを判定する /
* Hook to determine if an object is useable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 利用可能ならばTRUEを返す
*/
bool item_tester_hook_use(object_type *o_ptr)
{
	u32b flgs[TR_FLAG_SIZE];

	/* Ammo */
	if (o_ptr->tval == p_ptr->tval_ammo)
		return (TRUE);

	/* Useable object */
	switch (o_ptr->tval)
	{
	case TV_SPIKE:
	case TV_STAFF:
	case TV_WAND:
	case TV_ROD:
	case TV_SCROLL:
	case TV_POTION:
	case TV_FOOD:
	{
		return (TRUE);
	}

	default:
	{
		int i;

		/* Not known */
		if (!object_is_known(o_ptr)) return (FALSE);

		/* HACK - only items from the equipment can be activated */
		for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			if (&inventory[i] == o_ptr)
			{
				/* Extract the flags */
				object_flags(o_ptr, flgs);

				/* Check activation flag */
				if (have_flag(flgs, TR_ACTIVATE)) return (TRUE);
			}
		}
	}
	}

	/* Assume not */
	return (FALSE);
}


/*!
* @brief オブジェクトをプレイヤーが飲むことができるかを判定する /
* Hook to determine if an object can be quaffed
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 飲むことが可能ならばTRUEを返す
*/
bool item_tester_hook_quaff(object_type *o_ptr)
{
	if (o_ptr->tval == TV_POTION) return TRUE;

	if (prace_is_(RACE_ANDROID))
	{
		if (o_ptr->tval == TV_FLASK && o_ptr->sval == SV_FLASK_OIL)
			return TRUE;
	}
	return FALSE;
}


/*!
* @brief オブジェクトをプレイヤーが読むことができるかを判定する /
* Hook to determine if an object is readable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 読むことが可能ならばTRUEを返す
*/
bool item_tester_hook_readable(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_SCROLL) || (o_ptr->tval == TV_PARCHMENT) || (o_ptr->name1 == ART_GHB) || (o_ptr->name1 == ART_POWER)) return (TRUE);

	/* Assume not */
	return (FALSE);
}


/*!
* @brief エッセンスの付加可能な武器や矢弾かを返す
* @param o_ptr チェックしたいオブジェクトの構造体参照ポインタ
* @return エッセンスの付加可能な武器か矢弾ならばTRUEを返す。
*/
bool item_tester_hook_melee_ammo(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	case TV_BOLT:
	case TV_ARROW:
	case TV_SHOT:
	{
		return (TRUE);
	}
	case TV_SWORD:
	{
		if (o_ptr->sval != SV_DOKUBARI) return (TRUE);
	}
	}

	return (FALSE);
}

/*!
* @brief 呪術領域の武器呪縛の対象にできる武器かどうかを返す。 / An "item_tester_hook" for offer
* @param o_ptr オブジェクト構造体の参照ポインタ
* @return 呪縛可能な武器ならばTRUEを返す
*/
bool item_tester_hook_weapon_except_bow(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	{
		return (TRUE);
	}
	}

	return (FALSE);
}

/*!
* @brief 呪術領域の各処理に使える呪われた装備かどうかを返す。 / An "item_tester_hook" for offer
* @param o_ptr オブジェクト構造体の参照ポインタ
* @return 使える装備ならばTRUEを返す
*/
bool item_tester_hook_cursed(object_type *o_ptr)
{
	return (bool)(object_is_cursed(o_ptr));
}


/*!
* @brief アイテムが並の価値のアイテムかどうか判定する /
* Check if an object is nameless weapon or armour
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 並ならばTRUEを返す
*/
bool item_tester_hook_nameless_weapon_armour(object_type *o_ptr)
{
	/* Require weapon or armour */
	if (!object_is_weapon_armour_ammo(o_ptr)) return FALSE;

	/* Require nameless object if the object is well known */
	if (object_is_known(o_ptr) && !object_is_nameless(o_ptr))
		return FALSE;

	return TRUE;
}


/*!
* @brief アイテムが鑑定済みかを判定する /
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 実際に鑑定済みならばTRUEを返す
*/
bool item_tester_hook_identify(object_type *o_ptr)
{
	return (bool)!object_is_known(o_ptr);
}

/*!
* @brief アイテムが鑑定済みの武器防具かを判定する /
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 実際に鑑定済みならばTRUEを返す
*/
bool item_tester_hook_identify_weapon_armour(object_type *o_ptr)
{
	if (object_is_known(o_ptr))
		return FALSE;
	return object_is_weapon_armour_ammo(o_ptr);
}

/*!
* @brief アイテムが*鑑定*済みかを判定する /
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 実際に鑑定済みならばTRUEを返す
*/
bool item_tester_hook_identify_fully(object_type *o_ptr)
{
	return (bool)(!object_is_known(o_ptr) || !(o_ptr->ident & IDENT_MENTAL));
}

/*!
* @brief アイテムが*鑑定*済みの武器防具かを判定する /
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 実際に鑑定済みならばTRUEを返す
*/
bool item_tester_hook_identify_fully_weapon_armour(object_type *o_ptr)
{
	if (!item_tester_hook_identify_fully(o_ptr))
		return FALSE;
	return object_is_weapon_armour_ammo(o_ptr);
}


/*!
* @brief 魔力充填が可能なアイテムかどうか判定する /
* Hook for "get_item()".  Determine if something is rechargable.
* @param o_ptr 判定するアイテムの情報参照ポインタ
* @return 魔力充填が可能ならばTRUEを返す
*/
bool item_tester_hook_recharge(object_type *o_ptr)
{
	/* Recharge staffs */
	if (o_ptr->tval == TV_STAFF) return (TRUE);

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return (TRUE);

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return (TRUE);

	/* Nope */
	return (FALSE);
}
