#include "system/alloc-entries.h"
#include "system/baseitem-info.h"
#include "system/monster-race-info.h"

/* The entries in the "race allocator table" */
std::vector<alloc_entry> alloc_race_table;

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
    const auto &monraces = MonraceList::get_instance();
    std::vector<const MonsterRaceInfo *> elements;
    for (const auto &[monrace_id, monrace] : monraces_info) {
        if (monrace.is_valid()) {
            elements.push_back(&monrace);
        }
    }

    std::stable_sort(elements.begin(), elements.end(), [](const auto *r_ptr1, const auto *r_ptr2) {
        return r_ptr2->order_level_strictly(*r_ptr1);
    });
    this->entries.reserve(monraces.size());
    for (const auto *r_ptr : elements) {
        if (r_ptr->rarity == 0) { //!< ここ要検討、jsoncロード時に弾くべき.
            continue;
        }

        const auto index = static_cast<short>(r_ptr->idx);
        const auto level = r_ptr->level;
        const auto prob = static_cast<short>(100 / r_ptr->rarity);
        this->entries.emplace_back(index, level, prob, prob);
    }
}

const alloc_entry &MonraceAllocationTable::get_entry(int index) const
{
    return this->entries.at(index);
}

alloc_entry &MonraceAllocationTable::get_entry(int index)
{
    return this->entries.at(index);
}

/* The entries in the "kind allocator table" */
std::vector<alloc_entry> alloc_kind_table;

BaseitemInfo &alloc_entry::get_baseitem() const
{
    return BaseitemList::get_instance().get_baseitem(this->index);
}
