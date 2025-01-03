/*!
 * @brief ベースアイテムの確率分布及び選択処理定義
 * @author Hourier
 * @date 2024/12/03
 */

#pragma once

#include "util/abstract-vector-wrapper.h"
#include "util/probability-table.h"

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 */
class BaseitemKey;
class BaseitemDefinition;
class BaseitemAllocationEntry {
public:
    BaseitemAllocationEntry() = default;
    BaseitemAllocationEntry(short index, int level, short prob1, short prob2);
    short index; /* The actual index */
    int level; /* Base dungeon level */
    short prob1; /* Probability, pass 1 */
    short prob2; /* Probability, pass 2 */
    bool is_same_bi_key(const BaseitemKey &bi_key) const;
    bool is_chest() const;
    bool is_gold() const;
    int get_baseitem_level() const;
    bool order_level(const BaseitemAllocationEntry &other) const;

private:
    const BaseitemDefinition &get_baseitem() const;
    const BaseitemKey &get_bi_key() const;
};

class BaseitemAllocationTable : public util::AbstractVectorWrapper<BaseitemAllocationEntry> {
public:
    BaseitemAllocationTable(const BaseitemAllocationTable &) = delete;
    BaseitemAllocationTable(BaseitemAllocationTable &&) = delete;
    BaseitemAllocationTable operator=(const BaseitemAllocationTable &) = delete;
    BaseitemAllocationTable operator=(BaseitemAllocationTable &&) = delete;
    static BaseitemAllocationTable &get_instance();

    void initialize();
    const BaseitemAllocationEntry &get_entry(int index) const;
    BaseitemAllocationEntry &get_entry(int index);
    short draw_lottery(int level, uint32_t mode, int count) const;
    bool order_level(int index1, int index2) const;

    void prepare_allocation();

private:
    static BaseitemAllocationTable instance;
    BaseitemAllocationTable() = default;
    std::vector<BaseitemAllocationEntry> entries;

    std::vector<BaseitemAllocationEntry> &get_inner_container() override
    {
        return this->entries;
    }

    ProbabilityTable<int> make_table(int level, uint32_t mode) const;
};
