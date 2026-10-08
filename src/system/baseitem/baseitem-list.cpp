/*!
 * @brief ベースアイテムの集合論的処理実装
 * @author Hourier
 * @date 2024/11/16
 */

#include "system/baseitem/baseitem-list.h"
#include "system/angband-exceptions.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <numeric>
#include <range/v3/view.hpp>
#include <set>
#include <span>
#include <utility>

namespace {
constexpr auto INVALID_BI_ID_FORMAT = "Invalid Baseitem ID is specified! {}";
constexpr auto INVALID_BASEITEM_KEY = "Invalid Baseitem Key is specified! Type: {}, Subtype: {}";
}

BaseitemList BaseitemList::instance{};

BaseitemList::BaseitemList() = default;

BaseitemList::~BaseitemList() = default;

BaseitemList &BaseitemList::get_instance()
{
    return instance;
}

bool BaseitemList::is_valid(short bi_id) const
{
    if ((bi_id <= 0) || (static_cast<size_t>(bi_id) >= this->size())) {
        return false;
    }

    return this->baseitems.at(bi_id).is_valid();
}

BaseitemDefinition &BaseitemList::get_baseitem(short bi_id)
{
    this->validate(bi_id);
    return this->baseitems[bi_id];
}

const BaseitemDefinition &BaseitemList::get_baseitem(short bi_id) const
{
    this->validate(bi_id);
    return this->baseitems.at(bi_id);
}

const BaseitemDefinition &BaseitemList::pick_one_at_random() const
{
    const auto candidates = std::span(this->baseitems).subspan(1); // 0番はダミーアイテム
    return rand_choice(candidates);
}

void BaseitemList::resize(size_t new_size)
{
    this->baseitems.resize(new_size);
    this->invalidate_caches();
}

void BaseitemList::replace_baseitem(short bi_id, BaseitemDefinition &&baseitem)
{
    this->validate(bi_id);
    auto &target = this->baseitems[bi_id];
    std::destroy_at(std::addressof(target));
    std::construct_at(std::addressof(target), std::move(baseitem));
    this->invalidate_caches();
}

std::vector<short> BaseitemList::collect_valid_bi_ids() const
{
    if (this->valid_bi_ids_ready) {
        return this->valid_bi_ids;
    }

    std::vector<short> bi_ids;
    for (const auto &[bi_id, baseitem] : this->baseitems | ranges::views::enumerate) {
        if (baseitem.is_valid()) {
            bi_ids.push_back(static_cast<short>(bi_id));
        }
    }

    this->valid_bi_ids = std::move(bi_ids);
    this->valid_bi_ids_ready = true;
    return this->valid_bi_ids;
}

void BaseitemList::invalidate_caches()
{
    this->valid_bi_ids_ready = false;
    this->baseitem_keys_cache_ready = false;
    this->baseitem_subtypes_cache_ready = false;
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

    const auto &cache = this->create_baseitem_subtypes_cache();
    const auto it = cache.find(bi_key.tval());
    if (it == cache.end()) {
        constexpr auto fmt = "Specified ItemKindType has no subtype! %d";
        THROW_EXCEPTION(std::logic_error, format(fmt, enum2i(bi_key.tval())));
    }

    const auto &svals = it->second;
    return exe_lookup({ bi_key.tval(), rand_choice(svals) });
}

const BaseitemDefinition &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key) const
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}

void BaseitemList::validate(short bi_id) const
{
    if ((bi_id < 0) || (static_cast<size_t>(bi_id) >= this->size())) {
        THROW_EXCEPTION(std::logic_error, fmt::format(INVALID_BI_ID_FORMAT, bi_id));
    }
}

/*!
 * @brief ベースアイテムキーに対応するベースアイテムのIDを検索する
 * @param key 検索したいベースアイテムキー
 * @return ベースアイテムID
 * @details ベースアイテムIDが存在しなければ例外
 */
short BaseitemList::exe_lookup(const BaseitemKey &bi_key) const
{
    const auto &cache = this->create_baseitem_keys_cache();
    const auto it = cache.find(bi_key);
    if (it == cache.end()) {
        THROW_EXCEPTION(std::runtime_error, fmt::format(INVALID_BASEITEM_KEY, enum2i(bi_key.tval()), *bi_key.sval()));
    }

    return it->second;
}

/*
 * @brief tvalとbi_key.svalに対応する、BaseitenDefinitions のIDを返すためのキャッシュを生成する
 * @return tvalと(実在する)svalの組み合わせをキーに、ベースアイテムIDを値とした辞書
 */
const std::map<BaseitemKey, short> &BaseitemList::create_baseitem_keys_cache() const
{
    if (this->baseitem_keys_cache_ready) {
        return this->baseitem_keys_cache;
    }
    std::map<BaseitemKey, short> cache;
    for (short bi_id : this->collect_valid_bi_ids()) {
        const auto &bi_key = this->baseitems.at(bi_id).bi_key;
        cache[bi_key] = bi_id;
    }

    this->baseitem_keys_cache = std::move(cache);
    this->baseitem_keys_cache_ready = true;
    return this->baseitem_keys_cache;
}

/*
 * @brief 特定のtvalとランダムなsvalの組み合わせからベースアイテムを選択するためのキャッシュを生成する
 * @return tvalをキーに、svalのリストを値とした辞書
 */
const std::map<ItemKindType, std::vector<int>> &BaseitemList::create_baseitem_subtypes_cache() const
{
    if (this->baseitem_subtypes_cache_ready) {
        return this->baseitem_subtypes_cache;
    }
    std::map<ItemKindType, std::vector<int>> cache;
    for (short bi_id : this->collect_valid_bi_ids()) {
        const auto &bi_key = this->baseitems.at(bi_id).bi_key;
        const auto tval = bi_key.tval();
        cache[tval].push_back(*bi_key.sval());
    }

    this->baseitem_subtypes_cache = std::move(cache);
    this->baseitem_subtypes_cache_ready = true;
    return this->baseitem_subtypes_cache;
}

BaseitemDefinition &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key)
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}
