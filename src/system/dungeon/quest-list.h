#pragma once

#include "util/abstract-map-wrapper.h"
#include <cstdint>
#include <vector>

enum class MonraceId : short;
enum class QuestId : short;
enum class QuestKindType : short;
class QuestType;
class QuestList final : public util::AbstractMapWrapper<QuestId, QuestType> {
public:
    QuestList(const QuestList &) = delete;
    QuestList(QuestList &&) = delete;
    QuestList &operator=(const QuestList &) = delete;
    QuestList &operator=(QuestList &&) = delete;
    static QuestList &get_instance();

    void initialize();
    void reset_all();
    QuestType &get_quest(QuestId id);
    const QuestType &get_quest(QuestId id) const;
    std::vector<QuestId> get_sorted_quest_ids() const;

    void set_defeated_monster(QuestId id, short numbers);
    void set_max_monster(QuestId id, short numbers);
    void set_type(QuestId id, QuestKindType type);
    void set_monrace_id(QuestId id, MonraceId monrace_id);
    void set_flags(QuestId id, uint32_t flags);
    bool is_quest_equals(QuestId id, QuestKindType type) const;
    bool is_bounty_valid(QuestId id) const;

private:
    static QuestList instance;
    std::map<QuestId, QuestType> quests;
    QuestList() = default;

    std::map<QuestId, QuestType> &get_inner_container() override
    {
        return this->quests;
    }

    bool order_completed(QuestId id1, QuestId id2) const;
};
