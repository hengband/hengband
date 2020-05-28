#include "system/angband.h"
#include "util/util.h"
#include "object/artifact.h"
#include "object/item-feeling.h"
#include "object/object-hook.h"
#include "object/object-kind.h"
#include "object/special-object-flags.h"
#include "object/tr-types.h"
#include "monster/monster.h"
#include "player/player-class.h"
#include "player/player-skill.h"
#include "player/mimic-info-table.h"
#include "dungeon/quest.h"
#include "world/world.h"
#include "view/display-main-window.h"
#include "player/player-races-table.h"

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
	return FALSE;
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
		return TRUE;
	}
	case TV_SWORD:
	{
		if (o_ptr->sval != SV_POISON_NEEDLE) return TRUE;
	}
	}

	return FALSE;
}

/*!
* @brief オブジェクトが右手か左手に装備できる武器かどうかの判定
* @param o_ptr 判定するオブジェクトの構造体参照ポインタ
* @return 右手か左手の武器として装備できるならばTRUEを返す。
*/
bool item_tester_hook_melee_weapon(object_type *o_ptr)
{
	/* Check for a usable slot */
	if ((o_ptr->tval >= TV_DIGGING) && (o_ptr->tval <= TV_SWORD)) return TRUE;
	return FALSE;
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
		return TRUE;
	}
	}

	return FALSE;
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
	if ((o_ptr->tval == TV_DIGGING) || (o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM) || (o_ptr->tval == TV_HAFTED)) return TRUE;
	return FALSE;
}

/*!
* todo ここにplayer_type を追加すると関数ポインタの収拾がつかなくなったので保留
* @brief オブジェクトをプレイヤーが食べることができるかを判定する /
* Hook to determine if an object is eatable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 食べることが可能ならばTRUEを返す
*/
bool item_tester_hook_eatable(object_type *o_ptr)
{
	if (o_ptr->tval == TV_FOOD) return TRUE;

	if (PRACE_IS_(p_ptr, RACE_SKELETON) ||
		PRACE_IS_(p_ptr, RACE_GOLEM) ||
		PRACE_IS_(p_ptr, RACE_ZOMBIE) ||
		PRACE_IS_(p_ptr, RACE_SPECTRE))
	{
		if (o_ptr->tval == TV_STAFF || o_ptr->tval == TV_WAND)
			return TRUE;
	}
	else if (PRACE_IS_(p_ptr, RACE_BALROG) || (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_DEMON))
	{
		if (o_ptr->tval == TV_CORPSE &&
			o_ptr->sval == SV_CORPSE &&
			my_strchr("pht", r_info[o_ptr->pval].d_char))
			return TRUE;
	}

	return FALSE;
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
		(o_ptr->tval == TV_CARD)) return TRUE;
	return FALSE;
}

/*!
* @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
* Hook to determine if an object is activatable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 魔道具として発動可能ならばTRUEを返す
*/
bool item_tester_hook_activate(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];

	/* Not known */
	if (!object_is_known(o_ptr)) return FALSE;
	object_flags(o_ptr, flgs);

	/* Check activation flag */
	if (have_flag(flgs, TR_ACTIVATE)) return TRUE;

	return FALSE;
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
	if (wield_slot(p_ptr, o_ptr) >= INVEN_RARM) return TRUE;

	return FALSE;
}


/*!
* @brief オブジェクトをプレイヤーが簡易使用コマンドで利用できるかを判定する /
* Hook to determine if an object is useable
* @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
* @return 利用可能ならばTRUEを返す
*/
bool item_tester_hook_use(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];

	/* Ammo */
	if (o_ptr->tval == p_ptr->tval_ammo)
		return TRUE;

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
		return TRUE;
	}

	default:
	{
		int i;

		/* Not known */
		if (!object_is_known(o_ptr)) return FALSE;

		/* HACK - only items from the equipment can be activated */
		for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			if (&p_ptr->inventory_list[i] == o_ptr)
			{
				object_flags(o_ptr, flgs);

				/* Check activation flag */
				if (have_flag(flgs, TR_ACTIVATE)) return TRUE;
			}
		}
	}
	}

	return FALSE;
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

	if (PRACE_IS_(p_ptr, RACE_ANDROID))
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
	if ((o_ptr->tval == TV_SCROLL) || (o_ptr->tval == TV_PARCHMENT) || (o_ptr->name1 == ART_GHB) || (o_ptr->name1 == ART_POWER)) return TRUE;
	return FALSE;
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
		return TRUE;
	}
	case TV_SWORD:
	{
		if (o_ptr->sval != SV_POISON_NEEDLE) return TRUE;
	}
	}

	return FALSE;
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
		return TRUE;
	}
	}

	return FALSE;
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
	return (bool)(!object_is_known(o_ptr) || !OBJECT_IS_FULL_KNOWN(o_ptr));
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
	if (o_ptr->tval == TV_STAFF) return TRUE;

	/* Recharge wands */
	if (o_ptr->tval == TV_WAND) return TRUE;

	/* Hack -- Recharge rods */
	if (o_ptr->tval == TV_ROD) return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクトがプレイヤーが使用可能な魔道書かどうかを判定する
 * @param o_ptr 判定したいオブ会ジェクトの構造体参照ポインタ
 * @return 学習できる魔道書ならばTRUEを返す
 */
bool item_tester_learn_spell(object_type *o_ptr)
{
	s32b choices = realm_choices2[p_ptr->pclass];

	if (p_ptr->pclass == CLASS_PRIEST)
	{
		if (is_good_realm(p_ptr->realm1))
		{
			choices &= ~(CH_DEATH | CH_DAEMON);
		}
		else
		{
			choices &= ~(CH_LIFE | CH_CRUSADE);
		}
	}

	if ((o_ptr->tval < TV_LIFE_BOOK) || (o_ptr->tval > (TV_LIFE_BOOK + MAX_REALM - 1))) return FALSE;
	if ((o_ptr->tval == TV_MUSIC_BOOK) && (p_ptr->pclass == CLASS_BARD)) return TRUE;
	else if (!is_magic(tval2realm(o_ptr->tval))) return FALSE;
	if ((REALM1_BOOK == o_ptr->tval) || (REALM2_BOOK == o_ptr->tval)) return TRUE;
	if (choices & (0x0001 << (tval2realm(o_ptr->tval) - 1))) return TRUE;
	return FALSE;
}

/*!
 * @brief オブジェクトが高位の魔法書かどうかを判定する
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが高位の魔法書ならばTRUEを返す
 */
bool item_tester_high_level_book(object_type *o_ptr)
{
	if ((o_ptr->tval == TV_LIFE_BOOK) ||
		(o_ptr->tval == TV_SORCERY_BOOK) ||
		(o_ptr->tval == TV_NATURE_BOOK) ||
		(o_ptr->tval == TV_CHAOS_BOOK) ||
		(o_ptr->tval == TV_DEATH_BOOK) ||
		(o_ptr->tval == TV_TRUMP_BOOK) ||
		(o_ptr->tval == TV_CRAFT_BOOK) ||
		(o_ptr->tval == TV_DAEMON_BOOK) ||
		(o_ptr->tval == TV_CRUSADE_BOOK) ||
		(o_ptr->tval == TV_MUSIC_BOOK) ||
		(o_ptr->tval == TV_HEX_BOOK))
	{
		if (o_ptr->sval > 1)
			return TRUE;
		else
			return FALSE;
	}

	return FALSE;
}

/*!
 * @brief オブジェクトがランタンの燃料になるかどうかを判定する
 * An "item_tester_hook" for refilling lanterns
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトがランタンの燃料になるならばTRUEを返す
 */
bool item_tester_refill_lantern(object_type *o_ptr)
{
	/* Flasks of oil are okay */
	if (o_ptr->tval == TV_FLASK) return TRUE;

	/* Laterns are okay */
	if ((o_ptr->tval == TV_LITE) &&
		(o_ptr->sval == SV_LITE_LANTERN)) return TRUE;

	/* Assume not okay */
	return FALSE;
}


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
bool object_is_bounty(object_type *o_ptr)
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
	for (i = 0; i < MAX_BOUNTY; i++)
		if (o_ptr->pval == current_world_ptr->bounty_r_idx[i]) break;
	if (i < MAX_BOUNTY) return TRUE;

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
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		if (!have_flag(flgs, TR_BLESSED) &&
			!(o_ptr->tval == TV_HAFTED))
			return FALSE;
		break;
	}

	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		/* Icky to wield? */
		if (!(s_info[p_ptr->pclass].w_max[o_ptr->tval - TV_WEAPON_BEGIN][o_ptr->sval]))
			return FALSE;
		break;

	case CLASS_BEASTMASTER:
	case CLASS_CAVALRY:
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		/* Is it known to be suitable to using while riding? */
		if (!(have_flag(flgs, TR_RIDING)))
			return FALSE;

		break;
	}

	case CLASS_NINJA:
		/* Icky to wield? */
		if (s_info[p_ptr->pclass].w_max[o_ptr->tval - TV_WEAPON_BEGIN][o_ptr->sval] <= WEAPON_EXP_BEGINNER)
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
	switch (o_ptr->tval)
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
			o_ptr->sval == SV_POISON_NEEDLE ||
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
	if (o_ptr->tval == TV_SWORD && o_ptr->sval == SV_POISON_NEEDLE) return TRUE;

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

/*!
 * @brief オブジェクトが松明に束ねられるかどうかを判定する
 * An "item_tester_hook" for refilling torches
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが松明に束ねられるならばTRUEを返す
 */
bool object_can_refill_torch(object_type *o_ptr)
{
	/* Torches are okay */
	if ((o_ptr->tval == TV_LITE) &&
		(o_ptr->sval == SV_LITE_TORCH)) return TRUE;

	/* Assume not okay */
	return FALSE;
}


/*!
 * @brief 破壊可能なアイテムかを返す /
 * Determines whether an object can be destroyed, and makes fake inscription.
 * @param o_ptr 破壊可能かを確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが破壊可能ならばTRUEを返す
 */
bool can_player_destroy_object(object_type *o_ptr)
{
	/* Artifacts cannot be destroyed */
	if (!object_is_artifact(o_ptr)) return TRUE;

	/* If object is unidentified, makes fake inscription */
	if (!object_is_known(o_ptr))
	{
		byte feel = FEEL_SPECIAL;

		/* Hack -- Handle icky artifacts */
		if (object_is_cursed(o_ptr) || object_is_broken(o_ptr)) feel = FEEL_TERRIBLE;

		/* Hack -- inscribe the artifact */
		o_ptr->feeling = feel;

		/* We have "felt" it (again) */
		o_ptr->ident |= (IDENT_SENSE);
		p_ptr->update |= (PU_COMBINE);
		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		return FALSE;
	}

	/* Identified artifact -- Nothing to do */
	return FALSE;
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(object_type *o_ptr)
{
	if (p_ptr->current_floor_ptr->inside_quest)
	{
		ARTIFACT_IDX a_idx = quest[p_ptr->current_floor_ptr->inside_quest].k_idx;
		if (a_idx)
		{
			artifact_type *a_ptr = &a_info[a_idx];
			if (!(a_ptr->gen_flags & TRG_INSTA_ART))
			{
				if ((o_ptr->tval == a_ptr->tval) && (o_ptr->sval == a_ptr->sval))
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/*
 * Here is a "hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
bool(*item_tester_hook)(object_type*);

/*!
 * @brief アイテムがitem_tester_hookグローバル関数ポインタの条件を満たしているかを返す汎用関数
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return item_tester_hookの参照先、その他いくつかの例外に応じてTRUE/FALSEを返す。
 */
bool item_tester_okay(player_type *player_ptr, object_type *o_ptr, tval_type tval)
{
	/* Hack -- allow listing empty slots */
	// if (item_tester_full) return TRUE; // TODO:DELETE

	/* Require an item */
	if (!o_ptr->k_idx) return FALSE;

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD)
	{
		/* See xtra2.c */
		extern bool show_gold_on_floor;

		if (!show_gold_on_floor) return FALSE;
	}

	/* Check the tval */
	if (tval)
	{
		/* Is it a spellbook? If so, we need a hack -- TY */
		if ((tval <= TV_DEATH_BOOK) && (tval >= TV_LIFE_BOOK))
			return check_book_realm(player_ptr, o_ptr->tval, o_ptr->sval);
		else
			if (tval != o_ptr->tval) return FALSE;
	}

	/* Check the hook */
	if (item_tester_hook)
	{
		if (!(*item_tester_hook)(o_ptr)) return FALSE;
	}

	/* Assume okay */
	return TRUE;
}
