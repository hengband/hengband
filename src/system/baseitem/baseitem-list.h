/*!
 * @brief ベースアイテムの集合論的処理定義
 * @author Hourier
 * @date 2024/11/16
 */

#pragma once

#include <map>
#include <optional>
#include <vector>

enum class MoneyKind {
    COPPER,
    SILVER,
    GARNET,
    GOLD,
    OPAL,
    SAPPHIRE,
    RUBY,
    DIAMOND,
    EMERALD,
    MITHRIL,
    ADAMANTITE,
    MAX,
};

enum class ItemKindType : short;
enum class MonraceId : short;
class BaseitemDefinition;
class BaseitemKey;
class BaseitemList {
public:
    BaseitemList(BaseitemList &&) = delete;
    BaseitemList(const BaseitemList &) = delete;
    BaseitemList &operator=(const BaseitemList &) = delete;
    BaseitemList &operator=(BaseitemList &&) = delete;
    ~BaseitemList() = default;

    static BaseitemList &get_instance();
    BaseitemDefinition &get_baseitem(const short bi_id);
    const BaseitemDefinition &get_baseitem(const short bi_id) const;

    std::vector<BaseitemDefinition>::iterator begin();
    std::vector<BaseitemDefinition>::const_iterator begin() const;
    std::vector<BaseitemDefinition>::iterator end();
    std::vector<BaseitemDefinition>::const_iterator end() const;
    std::vector<BaseitemDefinition>::reverse_iterator rbegin();
    std::vector<BaseitemDefinition>::const_reverse_iterator rbegin() const;
    std::vector<BaseitemDefinition>::reverse_iterator rend();
    std::vector<BaseitemDefinition>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void resize(size_t new_size);
    void shrink_to_fit();

    std::optional<int> lookup_creeping_coin_drop_offset(MonraceId monrace_id) const;
    short lookup_baseitem_id(const BaseitemKey &bi_key) const;
    const BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key) const;
    int calc_num_gold_subtypes() const;
    const BaseitemDefinition &lookup_gold(int target_offset) const;
    int lookup_gold_offset(short bi_id) const;

    void reset_all_visuals();
    void reset_identification_flags();
    void mark_common_items_as_aware();
    void shuffle_flavors();

private:
    BaseitemList() = default;

    static BaseitemList instance;
    std::vector<BaseitemDefinition> baseitems{};

    short exe_lookup(const BaseitemKey &bi_key) const;
    const std::map<BaseitemKey, short> &create_baseitem_index_chache() const;
    const std::map<ItemKindType, std::vector<int>> &create_baseitems_cache() const;
    int lookup_gold_offset(const BaseitemKey &finding_bi_key) const;
    const std::map<MoneyKind, std::vector<BaseitemKey>> &create_sorted_golds() const;
    std::map<MoneyKind, std::vector<BaseitemKey>> create_unsorted_golds() const;

    BaseitemDefinition &lookup_baseitem(const BaseitemKey &bi_key);
    void shuffle_flavors(ItemKindType tval);
};
