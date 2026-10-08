/*!
 * @brief ベースアイテムの集合論的処理定義
 * @author Hourier
 * @date 2024/11/16
 */

#pragma once

#include "system/baseitem/baseitem-key.h"
#include "util/abstract-vector-wrapper.h"
#include <map>
#include <tl/optional.hpp>
#include <vector>

enum class ItemKindType : short;
enum class MonraceId : short;
class BaseitemDefinition;
class BaseitemKey;
class BaseitemList : public util::AbstractVectorWrapper<BaseitemDefinition> {
public:
    BaseitemList(BaseitemList &&) = delete;
    BaseitemList(const BaseitemList &) = delete;
    BaseitemList &operator=(const BaseitemList &) = delete;
    BaseitemList &operator=(BaseitemList &&) = delete;
    ~BaseitemList();

    static BaseitemList &get_instance();

    bool is_valid(short bi_id) const;
    BaseitemDefinition &get_baseitem(short bi_id); // 初期化専用.
    const BaseitemDefinition &get_baseitem(short bi_id) const;
    void resize(size_t new_size);
    // name/bi_keyの変更はこのメソッドで公開し、キャッシュを無効化する。
    void replace_baseitem(short bi_id, BaseitemDefinition &&baseitem);
    const BaseitemDefinition &pick_one_at_random() const;
    std::vector<short> collect_valid_bi_ids() const;

    short lookup_baseitem_id(const BaseitemKey &bi_key) const;
    const BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key) const;

private:
    BaseitemList();

    static BaseitemList instance;
    std::vector<BaseitemDefinition> baseitems;
    mutable std::vector<short> valid_bi_ids;
    mutable std::map<BaseitemKey, short> baseitem_keys_cache;
    mutable std::map<ItemKindType, std::vector<int>> baseitem_subtypes_cache;
    mutable bool valid_bi_ids_ready = false;
    mutable bool baseitem_keys_cache_ready = false;
    mutable bool baseitem_subtypes_cache_ready = false;

    std::vector<BaseitemDefinition> &get_inner_container() override
    {
        return this->baseitems;
    }

    void invalidate_caches();

    void validate(short bi_id) const;
    short exe_lookup(const BaseitemKey &bi_key) const;
    const std::map<BaseitemKey, short> &create_baseitem_keys_cache() const;
    const std::map<ItemKindType, std::vector<int>> &create_baseitem_subtypes_cache() const;

    BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key);
};
