/*!
 * @brief モンスター種族の集合論的処理定義
 * @author Hourier
 * @date 2024/12/03
 */

#pragma once

#include "util/abstract-map-wrapper.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

enum class MonraceId : short;

class MonraceDefinition;
extern std::map<MonraceId, MonraceDefinition> monraces_info;

class MonraceList : public util::AbstractMapWrapper<MonraceId, MonraceDefinition> {
public:
    MonraceList(MonraceList &&) = delete;
    MonraceList(const MonraceList &) = delete;
    MonraceList &operator=(const MonraceList &) = delete;
    MonraceList &operator=(MonraceList &&) = delete;

    static bool is_valid(MonraceId monrace_id);
    static const std::map<MonraceId, std::set<MonraceId>> &get_unified_uniques();
    static MonraceList &get_instance();
    static MonraceId empty_id();
    static bool is_tsuchinoko(MonraceId monrace_id);
    MonraceDefinition &emplace(MonraceId monrace_id);
    std::map<MonraceId, MonraceDefinition> &get_raw_map();
    MonraceDefinition &get_monrace(MonraceId monrace_id);
    const MonraceDefinition &get_monrace(MonraceId monrace_id) const;
    const std::vector<MonraceId> &get_valid_monrace_ids() const;
    const std::vector<std::pair<MonraceId, const MonraceDefinition *>> &get_sorted_monraces() const;
    bool can_unify_separate(const MonraceId r_idx) const;
    void kill_unified_unique(const MonraceId r_idx);
    bool is_selectable(const MonraceId r_idx) const;
    void defeat_separated_uniques();
    bool is_unified(const MonraceId r_idx) const;
    bool exists_separates(const MonraceId r_idx) const;
    bool is_separated(const MonraceId r_idx) const;
    bool can_select_separate(const MonraceId morace_id, const int hp, const int maxhp) const;
    bool order(MonraceId id1, MonraceId id2, bool is_detailed = false) const;
    bool order_level(MonraceId id1, MonraceId id2) const;
    bool order_level_unique(MonraceId id1, MonraceId id2) const;
    MonraceId pick_id_at_random() const;
    const MonraceDefinition &pick_monrace_at_random() const;
    int calc_defeat_count() const;

    void reset_current_numbers();
    void reset_all_visuals();
    std::optional<std::string> probe_lore(MonraceId monrace_id);
    void kill_unique_monster(MonraceId monrace_id);

private:
    MonraceList() = default;

    static MonraceList instance;

    const static std::map<MonraceId, std::set<MonraceId>> unified_uniques;

    std::map<MonraceId, MonraceDefinition> &get_inner_container() override
    {
        return monraces_info;
    }
};
