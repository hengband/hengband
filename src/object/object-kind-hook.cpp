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
#include "system/baseitem-info.h"
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
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::CLOAK;
}

/*!
 * @brief オブジェクトが竿状武器かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが竿状武器ならばTRUEを返す
 */
bool kind_is_polearm(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::POLEARM;
}

/*!
 * @brief オブジェクトが剣かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが剣ならばTRUEを返す
 */
bool kind_is_sword(short bi_id)
{
    const auto &k_ref = baseitems_info[bi_id];
    return (k_ref.bi_key.tval() == ItemKindType::SWORD) && (k_ref.bi_key.sval() > 2);
}

/*!
 * @brief オブジェクトが魔法書かどうかを判定する
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが魔法書ならばTRUEを返す
 */
bool kind_is_book(short bi_id)
{
    const auto &k_ref = baseitems_info[bi_id];
    return k_ref.bi_key.is_spell_book();
}

/*!
 * @brief オブジェクトがベースアイテム時点でGOODかどうかを判定する
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがベースアイテム時点でGOODなアイテムならばTRUEを返す
 */
bool kind_is_good_book(short bi_id)
{
    const auto &k_ref = baseitems_info[bi_id];
    return k_ref.bi_key.is_high_level_book();
}

/*!
 * @brief オブジェクトが鎧かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが鎧ならばTRUEを返す
 */
bool kind_is_armor(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::HARD_ARMOR;
}

/*!
 * @brief オブジェクトが打撃武器かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが打撃武器ならばTRUEを返す
 */
bool kind_is_hafted(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::HAFTED;
}

/*!
 * @brief オブジェクトが薬かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが薬ならばTRUEを返す
 */
bool kind_is_potion(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::POTION;
}

/*!
 * @brief オブジェクトが靴かどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが靴ならばTRUEを返す
 */
bool kind_is_boots(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::BOOTS;
}

/*!
 * @brief オブジェクトがアミュレットかどうかを判定する /
 * @param bi_id 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがアミュレットならばTRUEを返す
 */
bool kind_is_amulet(short bi_id)
{
    return baseitems_info[bi_id].bi_key.tval() == ItemKindType::AMULET;
}

/*!
 * @brief ベースアイテムが上質として扱われるかどうかを返す。
 * Hack -- determine if a template is "good"
 * @param bi_id 判定したいベースアイテムのID
 * @return ベースアイテムが上質ならばTRUEを返す。
 */
bool kind_is_good(short bi_id)
{
    const auto &k_ref = baseitems_info[bi_id];
    switch (k_ref.bi_key.tval()) {
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
        return k_ref.to_a >= 0;

    /* Weapons -- Good unless damaged */
    case ItemKindType::BOW:
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return (k_ref.to_h >= 0) && (k_ref.to_d >= 0);

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
        return k_ref.bi_key.sval() >= SV_BOOK_MIN_GOOD;

    /* Rings -- Rings of Speed are good */
    case ItemKindType::RING:
        return (k_ref.bi_key.sval() == SV_RING_SPEED) || (k_ref.bi_key.sval() == SV_RING_LORDLY);

    /* Amulets -- Amulets of the Magi and Resistance are good */
    case ItemKindType::AMULET:
        return (k_ref.bi_key.sval() == SV_AMULET_THE_MAGI) || (k_ref.bi_key.sval() == SV_AMULET_RESISTANCE);
    default:
        return false;
    }
}

/*
 * @brief 特定のtvalとランダムなsvalの組み合わせからベースアイテムを選択するためのキャッシュを生成する
 * @return tvalをキーに、svalのリストを値とした辞書
 */
static const std::map<ItemKindType, std::vector<int>> &create_baseitems_cache()
{
    static std::map<ItemKindType, std::vector<int>> cache;
    for (const auto &baseitem : baseitems_info) {
        const auto &bi_key = baseitem.bi_key;
        const auto tval = bi_key.tval();
        if (tval == ItemKindType::NONE) {
            continue;
        }

        cache[tval].push_back(bi_key.sval().value());
    }

    return cache;
}

/*
 * @brief tvalとbi_key.svalに対応する、BaseitenDefinitions のIDを返すためのキャッシュを生成する
 * @return tvalと(実在する)svalの組み合わせをキーに、ベースアイテムIDを値とした辞書
 */
static const std::map<BaseitemKey, short> &create_baseitem_index_chache()
{
    static std::map<BaseitemKey, short> cache;
    for (const auto &baseitem : baseitems_info) {
        const auto &bi_key = baseitem.bi_key;
        if (bi_key.tval() == ItemKindType::NONE) {
            continue;
        }

        cache[bi_key] = baseitem.idx;
    }

    return cache;
}

/*!
 * @brief tvalとsvalに対応するベースアイテムのIDを検索する
 * @param key 検索したいベースアイテムの、tval/svalのペア (svalがnulloptの可能性はない)
 * @return tvalとsvalに対応するベースアイテムが存在すればそのID、存在しなければ0
 * @details 存在しないことはリファクタリング成果により考えにくく、自作の不存在例外を投げればいいはず.
 * 但し呼び出し側全部の処理を保証するのが面倒なので旧処理のままとする.
 */
static short exe_lookup(const BaseitemKey &key)
{
    static const auto &cache = create_baseitem_index_chache();
    const auto itr = cache.find(key);
    if (itr == cache.end()) {
        return 0;
    }

    return itr->second;
}

/*!
 * @brief ベースアイテムのtval/svalからIDを引いて返す
 * @param key tval/svalのペア、但しsvalはランダム (nullopt)の可能性がある
 * @return ベースアイテムID
 * @details 「tvalが不存在」という状況は事実上ないが、辞書探索のお作法として「有無チェック」を入れておく.
 * 万が一tvalがキャッシュになかったらベースアイテムID 0を返す.
 */
short lookup_baseitem_id(const BaseitemKey &key)
{
    const auto sval = key.sval();
    if (sval.has_value()) {
        return exe_lookup(key);
    }

    static const auto &cache = create_baseitems_cache();
    const auto itr = cache.find(key.tval());
    if (itr == cache.end()) {
        return 0;
    }

    const auto &svals = itr->second;
    const auto sval_indice = randint0(svals.size());
    return exe_lookup({ key.tval(), svals.at(sval_indice) });
}
