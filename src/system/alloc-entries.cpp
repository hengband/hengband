#include "system/alloc-entries.h"
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"

MonraceAllocationEntry::MonraceAllocationEntry(MonsterRaceId index, int level, short prob1, short prob2)
    : index(index)
    , level(level)
    , prob1(prob1)
    , prob2(prob2)
{
}

const MonsterRaceInfo &MonraceAllocationEntry::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(index);
}

MonraceAllocationTable MonraceAllocationTable::instance{};

MonraceAllocationTable &MonraceAllocationTable::get_instance()
{
    return instance;
}

size_t MonraceAllocationTable::size() const
{
    return this->entries.size();
}

void MonraceAllocationTable::initialize()
{
    auto &monraces = MonraceList::get_instance();
    const auto &elements = monraces.get_sorted_monraces();
    this->entries.reserve(elements.size());
    for (const auto &[monrace_id, r_ptr] : elements) {
        if (r_ptr->rarity == 0) { //!< ここ要検討、jsoncロード時に弾くべき.
            continue;
        }

        const auto prob = static_cast<short>(100 / r_ptr->rarity);
        this->entries.emplace_back(monrace_id, r_ptr->level, prob, prob);
    }
}

std::vector<MonraceAllocationEntry>::iterator MonraceAllocationTable::begin()
{
    return this->entries.begin();
}

std::vector<MonraceAllocationEntry>::const_iterator MonraceAllocationTable::begin() const
{
    return this->entries.begin();
}

std::vector<MonraceAllocationEntry>::iterator MonraceAllocationTable::end()
{
    return this->entries.end();
}

std::vector<MonraceAllocationEntry>::const_iterator MonraceAllocationTable::end() const
{
    return this->entries.end();
}

const MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index) const
{
    return this->entries.at(index);
}

MonraceAllocationEntry &MonraceAllocationTable::get_entry(int index)
{
    return this->entries.at(index);
}

/* The entries in the "kind allocator table" */
std::vector<alloc_entry> alloc_kind_table;

BaseitemInfo &alloc_entry::get_baseitem() const
{
    return BaseitemList::get_instance().get_baseitem(this->index);
}
