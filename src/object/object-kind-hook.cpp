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
#include "system/baseitem/baseitem-list.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

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
    const auto &bi_key = baseitem.bi_key;

    // 防具は AC の修正値が負でなければ上質
    if (bi_key.is_protector()) {
        return baseitem.to_a >= 0;
    }

    // 武器は命中・ダメージの修正値が負でなければ上質
    if (bi_key.is_weapon()) {
        return (baseitem.to_h >= 0) && (baseitem.to_d >= 0);
    }

    // 魔法書は 3 冊目以降なら上質 (秘術の魔法書を除く)
    if (bi_key.is_spell_book()) {
        return bi_key.is_high_level_book();
    }

    switch (bi_key.tval()) {
    // 矢弾は矢・ボルトなら上質
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
        return true;

    // 指輪はスピード・王者の加護の指輪なら上質
    case ItemKindType::RING:
        return (bi_key.sval() == SV_RING_SPEED) || (bi_key.sval() == SV_RING_LORDLY);

    // アミュレットは賢者・耐性のアミュレットなら上質
    case ItemKindType::AMULET:
        return (bi_key.sval() == SV_AMULET_THE_MAGI) || (bi_key.sval() == SV_AMULET_RESISTANCE);
    default:
        return false;
    }
}
