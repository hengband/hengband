/*!
 * @brief ベースアイテムの集合論的処理実装
 * @author Hourier
 * @date 2024/11/16
 */

#include "system/baseitem/baseitem-list.h"
#include "object/tval-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-staff-types.h"
#include "system/angband-exceptions.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "util/string-processor.h"
#include <algorithm>
#include <numeric>
#include <set>

namespace {
constexpr auto INVALID_BI_ID_FORMAT = "Invalid Baseitem ID is specified! %d";
constexpr auto INVALID_BASEITEM_KEY = "Invalid Baseitem Key is specified! Type: %d, Subtype: %d";
}

BaseitemList BaseitemList::instance{};

BaseitemList &BaseitemList::get_instance()
{
    return instance;
}

BaseitemDefinition &BaseitemList::get_baseitem(const short bi_id)
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->baseitems.size()))) {
        THROW_EXCEPTION(std::logic_error, format(INVALID_BI_ID_FORMAT, bi_id));
    }

    return this->baseitems[bi_id];
}

const BaseitemDefinition &BaseitemList::get_baseitem(const short bi_id) const
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->baseitems.size()))) {
        THROW_EXCEPTION(std::logic_error, format(INVALID_BI_ID_FORMAT, bi_id));
    }

    return this->baseitems[bi_id];
}

std::vector<BaseitemDefinition>::iterator BaseitemList::begin()
{
    return this->baseitems.begin();
}

std::vector<BaseitemDefinition>::const_iterator BaseitemList::begin() const
{
    return this->baseitems.begin();
}

std::vector<BaseitemDefinition>::iterator BaseitemList::end()
{
    return this->baseitems.end();
}

std::vector<BaseitemDefinition>::const_iterator BaseitemList::end() const
{
    return this->baseitems.end();
}

std::vector<BaseitemDefinition>::reverse_iterator BaseitemList::rbegin()
{
    return this->baseitems.rbegin();
}

std::vector<BaseitemDefinition>::const_reverse_iterator BaseitemList::rbegin() const
{
    return this->baseitems.rbegin();
}

std::vector<BaseitemDefinition>::reverse_iterator BaseitemList::rend()
{
    return this->baseitems.rend();
}

std::vector<BaseitemDefinition>::const_reverse_iterator BaseitemList::rend() const
{
    return this->baseitems.rend();
}

size_t BaseitemList::size() const
{
    return this->baseitems.size();
}

bool BaseitemList::empty() const
{
    return this->baseitems.empty();
}

void BaseitemList::resize(size_t new_size)
{
    this->baseitems.resize(new_size);
}

void BaseitemList::shrink_to_fit()
{
    this->baseitems.shrink_to_fit();
}

/*!
 * @brief ベースアイテムキーからIDを引いて返す
 * @param key ベースアイテムキー、但しsvalはランダム(nullopt) の可能性がある
 * @return ベースアイテムID
 * @details ベースアイテムIDが存在しなければ例外
 */
short BaseitemList::lookup_baseitem_id(const BaseitemKey &bi_key) const
{
    const auto sval = bi_key.sval();
    if (sval) {
        return exe_lookup(bi_key);
    }

    static const auto &cache = this->create_baseitem_subtypes_cache();
    const auto it = cache.find(bi_key.tval());
    if (it == cache.end()) {
        constexpr auto fmt = "Specified ItemKindType has no subtype! %d";
        THROW_EXCEPTION(std::runtime_error, format(fmt, enum2i(bi_key.tval())));
    }

    const auto &svals = it->second;
    return exe_lookup({ bi_key.tval(), rand_choice(svals) });
}

const BaseitemDefinition &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key) const
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}

void BaseitemList::reset_all_visuals()
{
    for (auto &baseitem : this->baseitems) {
        baseitem.reset_visual();
    }
}

/*!
 * @brief ベースアイテムの鑑定済みフラグをリセットする
 * @details 不具合対策で0からリセットする(セーブは0から)
 */
void BaseitemList::reset_identification_flags()
{
    for (auto &baseitem : this->baseitems) {
        baseitem.mark_trial(false);
        baseitem.mark_awareness(false);
    }
}

/*!
 * @brief 未鑑定アイテム種別の内、ゲーム開始時から鑑定済とするアイテムの鑑定済フラグをONにする
 * @todo 食料用の杖は該当種族 (ゴーレム/骸骨/ゾンビ/幽霊)では鑑定済だが、本来はこのメソッドで鑑定済にすべき.
 */
void BaseitemList::mark_common_items_as_aware()
{
    std::vector<BaseitemKey> bi_keys;
    bi_keys.emplace_back(ItemKindType::POTION, SV_POTION_WATER);
    bi_keys.emplace_back(ItemKindType::STAFF, SV_STAFF_NOTHING);
    for (const auto &bi_key : bi_keys) {
        this->lookup_baseitem(bi_key).mark_awareness(true);
    }
}

void BaseitemList::shuffle_flavors()
{
    this->shuffle_flavors(ItemKindType::RING);
    this->shuffle_flavors(ItemKindType::AMULET);
    this->shuffle_flavors(ItemKindType::STAFF);
    this->shuffle_flavors(ItemKindType::WAND);
    this->shuffle_flavors(ItemKindType::ROD);
    this->shuffle_flavors(ItemKindType::FOOD);
    this->shuffle_flavors(ItemKindType::POTION);
    this->shuffle_flavors(ItemKindType::SCROLL);
}

/*!
 * @brief ベースアイテムキーに対応するベースアイテムのIDを検索する
 * @param key 検索したいベースアイテムキー
 * @return ベースアイテムID
 * @details ベースアイテムIDが存在しなければ例外
 */
short BaseitemList::exe_lookup(const BaseitemKey &bi_key) const
{
    static const auto &cache = this->create_baseitem_keys_cache();
    const auto it = cache.find(bi_key);
    if (it == cache.end()) {
        THROW_EXCEPTION(std::runtime_error, format(INVALID_BASEITEM_KEY, enum2i(bi_key.tval()), *bi_key.sval()));
    }

    return it->second;
}

/*
 * @brief tvalとbi_key.svalに対応する、BaseitenDefinitions のIDを返すためのキャッシュを生成する
 * @return tvalと(実在する)svalの組み合わせをキーに、ベースアイテムIDを値とした辞書
 */
const std::map<BaseitemKey, short> &BaseitemList::create_baseitem_keys_cache() const
{
    static std::map<BaseitemKey, short> cache;
    for (const auto &baseitem : this->baseitems) {
        if (baseitem.is_valid()) {
            const auto &bi_key = baseitem.bi_key;
            cache[bi_key] = baseitem.idx;
        }
    }

    return cache;
}

/*
 * @brief 特定のtvalとランダムなsvalの組み合わせからベースアイテムを選択するためのキャッシュを生成する
 * @return tvalをキーに、svalのリストを値とした辞書
 */
const std::map<ItemKindType, std::vector<int>> &BaseitemList::create_baseitem_subtypes_cache() const
{
    static std::map<ItemKindType, std::vector<int>> cache;
    for (const auto &baseitem : this->baseitems) {
        if (baseitem.is_valid()) {
            const auto &bi_key = baseitem.bi_key;
            const auto tval = bi_key.tval();
            cache[tval].push_back(*bi_key.sval());
        }
    }

    return cache;
}

BaseitemDefinition &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key)
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}

/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする
 * @param tval シャッフルしたいtval
 * @details 巻物、各種魔道具などに利用される。
 */
void BaseitemList::shuffle_flavors(ItemKindType tval)
{
    std::vector<std::reference_wrapper<short>> flavors;
    for (auto &baseitem : this->baseitems) {
        if (baseitem.bi_key.tval() != tval) {
            continue;
        }

        if (baseitem.flavor == 0) {
            continue;
        }

        if (baseitem.flags.has(TR_FIXED_FLAVOR)) {
            continue;
        }

        flavors.push_back(baseitem.flavor);
    }

    rand_shuffle(flavors.begin(), flavors.end());
}
