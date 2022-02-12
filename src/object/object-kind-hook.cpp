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
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがクロークならばTRUEを返す
 */
bool kind_is_cloak(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::CLOAK;
}

/*!
 * @brief オブジェクトが竿状武器かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが竿状武器ならばTRUEを返す
 */
bool kind_is_polearm(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::POLEARM;
}

/*!
 * @brief オブジェクトが剣かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが剣ならばTRUEを返す
 */
bool kind_is_sword(KIND_OBJECT_IDX k_idx)
{
    auto *k_ptr = &k_info[k_idx];
    return (k_ptr->tval == ItemKindType::SWORD) && (k_ptr->sval > 2);
}

/*!
 * @brief オブジェクトが魔法書かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが魔法書ならばTRUEを返す
 */
bool kind_is_book(KIND_OBJECT_IDX k_idx)
{
    auto *k_ptr = &k_info[k_idx];
    return (k_ptr->tval >= ItemKindType::LIFE_BOOK) && (k_ptr->tval <= ItemKindType::CRUSADE_BOOK);
}

/*!
 * @brief オブジェクトがベースアイテム時点でGOODかどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがベースアイテム時点でGOODなアイテムならばTRUEを返す
 */
bool kind_is_good_book(KIND_OBJECT_IDX k_idx)
{
    auto *k_ptr = &k_info[k_idx];
    return (k_ptr->tval >= ItemKindType::LIFE_BOOK) && (k_ptr->tval <= ItemKindType::CRUSADE_BOOK) && (k_ptr->tval != ItemKindType::ARCANE_BOOK) && (k_ptr->sval > 1);
}

/*!
 * @brief オブジェクトが鎧かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが鎧ならばTRUEを返す
 */
bool kind_is_armor(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::HARD_ARMOR;
}

/*!
 * @brief オブジェクトが打撃武器かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが打撃武器ならばTRUEを返す
 */
bool kind_is_hafted(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::HAFTED;
}

/*!
 * @brief オブジェクトが薬かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool kind_is_potion(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::POTION;
}

/*!
 * @brief オブジェクトが靴かどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが靴ならばTRUEを返す
 */
bool kind_is_boots(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::BOOTS;
}

/*!
 * @brief オブジェクトがアミュレットかどうかを判定する /
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがアミュレットならばTRUEを返す
 */
bool kind_is_amulet(KIND_OBJECT_IDX k_idx)
{
    return k_info[k_idx].tval == ItemKindType::AMULET;
}

/*!
 * @brief ベースアイテムが上質として扱われるかどうかを返す。
 * Hack -- determine if a template is "good"
 * @param k_idx 判定したいベースアイテムのID
 * @return ベースアイテムが上質ならばTRUEを返す。
 */
bool kind_is_good(KIND_OBJECT_IDX k_idx)
{
    auto *k_ptr = &k_info[k_idx];
    switch (k_ptr->tval) {
        /* Armor -- Good unless damaged */
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
        return k_ptr->to_a >= 0;

    /* Weapons -- Good unless damaged */
    case ItemKindType::BOW:
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return (k_ptr->to_h >= 0) && (k_ptr->to_d >= 0);

    /* Ammo -- Arrows/Bolts are good */
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
        return true;

    /* Books -- High level books are good (except Arcane books) */
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        return k_ptr->sval >= SV_BOOK_MIN_GOOD;

    /* Rings -- Rings of Speed are good */
    case ItemKindType::RING:
        return (k_ptr->sval == SV_RING_SPEED) || (k_ptr->sval == SV_RING_LORDLY);

    /* Amulets -- Amulets of the Magi and Resistance are good */
    case ItemKindType::AMULET:
        return (k_ptr->sval == SV_AMULET_THE_MAGI) || (k_ptr->sval == SV_AMULET_RESISTANCE);
    default:
        return false;
    }
}

/*!
 * @brief tvalとsvalに対応するベースアイテムのIDを返す。
 * Find the index of the object_kind with the given tval and sval
 * @param tval 検索したいベースアイテムのtval
 * @param sval 検索したいベースアイテムのsval
 */
KIND_OBJECT_IDX lookup_kind(ItemKindType tval, OBJECT_SUBTYPE_VALUE sval)
{
    int num = 0;
    KIND_OBJECT_IDX bk = 0;
    for (const auto& k_ref : k_info) {
        if (k_ref.tval != tval)
            continue;

        if (k_ref.sval == sval)
            return k_ref.idx;

        if ((sval != SV_ANY) || !one_in_(++num))
            continue;

        bk = k_ref.idx;
    }

    if (sval == SV_ANY)
        return bk;

    return 0;
}
