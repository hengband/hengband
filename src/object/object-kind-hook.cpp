/*!
 * @brief アイテムが特定種別のものであるかどうかの判定関数群
 * @date 2018/12/15
 * @author deskull
 */

#include "object/object-kind-hook.h"
#include "object/tval-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-ring-types.h"
#include "system/baseitem/baseitem-definition.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

// @brief 3冊目の魔法書からは上質アイテムとして扱う.
static const int SV_BOOK_MIN_GOOD = 2;

/*!
 * @brief オブジェクトがクロークかどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがクロークならばTRUEを返す
 */
bool kind_is_cloak(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::CLOAK;
}

/*!
 * @brief オブジェクトが竿状武器かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが竿状武器ならばTRUEを返す
 */
bool kind_is_polearm(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::POLEARM;
}

/*!
 * @brief オブジェクトが剣かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが剣ならばTRUEを返す
 */
bool kind_is_sword(short bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    return (baseitem.bi_key.tval() == ItemKindType::SWORD) && (baseitem.bi_key.sval() > 2);
}

/*!
 * @brief オブジェクトが魔法書かどうかを判定する
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが魔法書ならばTRUEを返す
 */
bool kind_is_book(short bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    return baseitem.bi_key.is_spell_book();
}

/*!
 * @brief オブジェクトがベースアイテム時点でGOODかどうかを判定する
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがベースアイテム時点でGOODなアイテムならばTRUEを返す
 */
bool kind_is_good_book(short bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    return baseitem.bi_key.is_high_level_book();
}

/*!
 * @brief オブジェクトが鎧かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが鎧ならばTRUEを返す
 */
bool kind_is_armor(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::HARD_ARMOR;
}

/*!
 * @brief オブジェクトが打撃武器かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが打撃武器ならばTRUEを返す
 */
bool kind_is_hafted(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::HAFTED;
}

/*!
 * @brief オブジェクトが薬かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool kind_is_potion(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::POTION;
}

/*!
 * @brief オブジェクトが靴かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが靴ならばTRUEを返す
 */
bool kind_is_boots(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::BOOTS;
}

/*!
 * @brief オブジェクトがアミュレットかどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがアミュレットならばTRUEを返す
 */
bool kind_is_amulet(short bi_id)
{
    return BaseitemList::get_instance().get_baseitem(bi_id).bi_key.tval() == ItemKindType::AMULET;
}

/*!
 * @brief ベースアイテムが上質として扱われるかどうかを返す。
 * Hack -- determine if a template is "good"
 * @param bi_id 判定したいベースアイテムのID
 * @return ベースアイテムが上質ならばTRUEを返す。
 */
bool kind_is_good(short bi_id)
{
    const auto &baseitem = BaseitemList::get_instance().get_baseitem(bi_id);
    switch (baseitem.bi_key.tval()) {
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
        return baseitem.to_a >= 0;

    /* Weapons -- Good unless damaged */
    case ItemKindType::BOW:
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return (baseitem.to_h >= 0) && (baseitem.to_d >= 0);

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
        return baseitem.bi_key.sval() >= SV_BOOK_MIN_GOOD;

    /* Rings -- Rings of Speed are good */
    case ItemKindType::RING:
        return (baseitem.bi_key.sval() == SV_RING_SPEED) || (baseitem.bi_key.sval() == SV_RING_LORDLY);

    /* Amulets -- Amulets of the Magi and Resistance are good */
    case ItemKindType::AMULET:
        return (baseitem.bi_key.sval() == SV_AMULET_THE_MAGI) || (baseitem.bi_key.sval() == SV_AMULET_RESISTANCE);
    default:
        return false;
    }
}
