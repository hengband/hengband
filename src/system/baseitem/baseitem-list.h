/*!
 * @brief ベースアイテムの集合論的処理定義
 * @author Hourier
 * @date 2024/11/16
 */

#pragma once

#include "util/abstract-vector-wrapper.h"
#include <map>
#include <optional>
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
    BaseitemDefinition &get_baseitem(const short bi_id);
    const BaseitemDefinition &get_baseitem(const short bi_id) const;

    short lookup_baseitem_id(const BaseitemKey &bi_key) const;
    const BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key) const;

    void reset_all_visuals();
    void reset_identification_flags();
    void mark_common_items_as_aware();
    void shuffle_flavors();

private:
    BaseitemList() = default;

    static BaseitemList instance;
    std::vector<BaseitemDefinition> baseitems;

    std::vector<BaseitemDefinition> &get_inner_container() override
    {
        return this->baseitems;
    }

    short exe_lookup(const BaseitemKey &bi_key) const;
    const std::map<BaseitemKey, short> &create_baseitem_keys_cache() const;
    const std::map<ItemKindType, std::vector<int>> &create_baseitem_subtypes_cache() const;

    BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key);
    void shuffle_flavors(ItemKindType tval);
};
