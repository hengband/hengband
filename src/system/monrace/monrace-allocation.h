/*
 * @brief モンスター種族の確率分布及び選択処理定義
 * @author Robert A. Koeneke, James E. Wilson, Ben Harrison, mogami, Hourier
 * @date 1983, 1990, 1997, 2002/01/12, 2024/12/03
 */

#pragma once

#include "util/abstract-vector-wrapper.h"
#include <vector>

enum class MonraceId : short;
class MonraceDefinition;
class MonraceAllocationEntry {
public:
    MonraceAllocationEntry() = default;
    MonraceAllocationEntry(MonraceId index, int level, short prob1, short prob2);
    MonraceId index{}; /* The actual index */
    int level; /* Base dungeon level */
    short prob1; /* Probability, pass 1 */
    short prob2; /* Probability, pass 2 */
    bool is_permitted(int threshold_level) const;
    bool is_defeatable(int threshold_level) const;

private:
    const MonraceDefinition &get_monrace() const;
};

class MonraceAllocationTable : public util::AbstractVectorWrapper<MonraceAllocationEntry> {
public:
    MonraceAllocationTable(const MonraceAllocationTable &) = delete;
    MonraceAllocationTable(MonraceAllocationTable &&) = delete;
    MonraceAllocationTable operator=(const MonraceAllocationTable &) = delete;
    MonraceAllocationTable operator=(MonraceAllocationTable &&) = delete;
    static MonraceAllocationTable &get_instance();

    void initialize();
    const MonraceAllocationEntry &get_entry(int index) const;
    MonraceAllocationEntry &get_entry(int index);

private:
    static MonraceAllocationTable instance;
    MonraceAllocationTable() = default;
    std::vector<MonraceAllocationEntry> entries{};

    std::vector<MonraceAllocationEntry> &get_inner_container() override
    {
        return this->entries;
    }
};
