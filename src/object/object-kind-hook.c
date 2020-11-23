/*!
 * @brief アイテムが特定種別のものであるかどうかの判定関数群
 * @date 2018/12/15
 * @author deskull
 */

#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"

/*
 * Special "sval" limit -- first "good" magic/prayer book
 */
static const int SV_BOOK_MIN_GOOD = 2;

/*!
 * @brief オブジェクトがクロークかどうかを判定する /
 * Hack -- determine if a template is Cloak
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがクロークならばTRUEを返す
 */
bool kind_is_cloak(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_CLOAK)
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトが竿状武器かどうかを判定する /
 * Hack -- determine if a template is Polearm
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが竿状武器ならばTRUEを返す
 */
bool kind_is_polearm(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_POLEARM)
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトが剣かどうかを判定する /
 * Hack -- determine if a template is Sword
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが剣ならばTRUEを返す
 */
bool kind_is_sword(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval == TV_SWORD) && (k_ptr->sval > 2))
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトが魔法書かどうかを判定する /
 * Hack -- determine if a template is Book
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが魔法書ならばTRUEを返す
 */
bool kind_is_book(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval >= TV_LIFE_BOOK) && (k_ptr->tval <= TV_CRUSADE_BOOK))
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトがベースアイテム時点でGOODかどうかを判定する /
 * Hack -- determine if a template is Good book
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがベースアイテム時点でGOODなアイテムならばTRUEを返す
 */
bool kind_is_good_book(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval >= TV_LIFE_BOOK) && (k_ptr->tval <= TV_CRUSADE_BOOK) && (k_ptr->tval != TV_ARCANE_BOOK) && (k_ptr->sval > 1))
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトが鎧かどうかを判定する /
 * Hack -- determine if a template is Armor
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが鎧ならばTRUEを返す
 */
bool kind_is_armor(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_HARD_ARMOR)
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}


/*!
 * @brief オブジェクトが打撃武器かどうかを判定する /
 * Hack -- determine if a template is hafted weapon
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが打撃武器ならばTRUEを返す
 */
bool kind_is_hafted(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_HAFTED)
	{
		return TRUE;
	}

	/* Assume not good */
	return FALSE;
}

/*
 * Hack -- determine if a template is potion
 */
bool kind_is_potion(KIND_OBJECT_IDX k_idx)
{
	return k_info[k_idx].tval == TV_POTION;
}

/*!
 * @brief ベースアイテムが上質として扱われるかどうかを返す。
 * Hack -- determine if a template is "good"
 * @param k_idx 判定したいベースアイテムのID
 * @return ベースアイテムが上質ならばTRUEを返す。
 */
bool kind_is_good(KIND_OBJECT_IDX k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	switch (k_ptr->tval)
	{
		/* Armor -- Good unless damaged */
	case TV_HARD_ARMOR:
	case TV_SOFT_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	{
		if (k_ptr->to_a < 0) return FALSE;
		return TRUE;
	}

	/* Weapons -- Good unless damaged */
	case TV_BOW:
	case TV_SWORD:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_DIGGING:
	{
		if (k_ptr->to_h < 0) return FALSE;
		if (k_ptr->to_d < 0) return FALSE;
		return TRUE;
	}

	/* Ammo -- Arrows/Bolts are good */
	case TV_BOLT:
	case TV_ARROW:
	{
		return TRUE;
	}

	/* Books -- High level books are good (except Arcane books) */
	case TV_LIFE_BOOK:
	case TV_SORCERY_BOOK:
	case TV_NATURE_BOOK:
	case TV_CHAOS_BOOK:
	case TV_DEATH_BOOK:
	case TV_TRUMP_BOOK:
	case TV_CRAFT_BOOK:
	case TV_DEMON_BOOK:
	case TV_CRUSADE_BOOK:
	case TV_MUSIC_BOOK:
	case TV_HISSATSU_BOOK:
	case TV_HEX_BOOK:
	{
		if (k_ptr->sval >= SV_BOOK_MIN_GOOD) return TRUE;
		return FALSE;
	}

	/* Rings -- Rings of Speed are good */
	case TV_RING:
	{
		if (k_ptr->sval == SV_RING_SPEED) return TRUE;
		if (k_ptr->sval == SV_RING_LORDLY) return TRUE;
		return FALSE;
	}

	/* Amulets -- Amulets of the Magi and Resistance are good */
	case TV_AMULET:
	{
		if (k_ptr->sval == SV_AMULET_THE_MAGI) return TRUE;
		if (k_ptr->sval == SV_AMULET_RESISTANCE) return TRUE;
		return FALSE;
	}
	}

	/* Assume not good */
	return FALSE;
}

/*!
 * @brief tvalとsvalに対応するベースアイテムのIDを返す。
 * Find the index of the object_kind with the given tval and sval
 * @param tval 検索したいベースアイテムのtval
 * @param sval 検索したいベースアイテムのsval
 * @return なし
 */
KIND_OBJECT_IDX lookup_kind(tval_type tval, OBJECT_SUBTYPE_VALUE sval)
{
    int num = 0;
    KIND_OBJECT_IDX bk = 0;

    for (KIND_OBJECT_IDX k = 1; k < max_k_idx; k++) {
        object_kind *k_ptr = &k_info[k];
        if (k_ptr->tval != tval)
            continue;
        if (k_ptr->sval == sval)
            return (k);
        if (sval != SV_ANY)
            continue;
        if (!one_in_(++num))
            continue;

        bk = k;
    }

    if (sval == SV_ANY) {
        return bk;
    }

    return 0;
}
