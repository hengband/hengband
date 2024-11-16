/*
 * @brief
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2002/01/12 mogami
 * 2021/10/02 Hourier
 */

#pragma once

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

class MonraceAllocationTable {
public:
    MonraceAllocationTable(const MonraceAllocationTable &) = delete;
    MonraceAllocationTable(MonraceAllocationTable &&) = delete;
    MonraceAllocationTable operator=(const MonraceAllocationTable &) = delete;
    MonraceAllocationTable operator=(MonraceAllocationTable &&) = delete;
    static MonraceAllocationTable &get_instance();

    void initialize();
    std::vector<MonraceAllocationEntry>::iterator begin();
    std::vector<MonraceAllocationEntry>::const_iterator begin() const;
    std::vector<MonraceAllocationEntry>::iterator end();
    std::vector<MonraceAllocationEntry>::const_iterator end() const;
    size_t size() const;
    const MonraceAllocationEntry &get_entry(int index) const;
    MonraceAllocationEntry &get_entry(int index);

private:
    static MonraceAllocationTable instance;
    MonraceAllocationTable() = default;
    std::vector<MonraceAllocationEntry> entries{};
};

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
    const BaseitemKey &get_bi_key() const;
    int get_baseitem_level() const;

private:
    const BaseitemDefinition &get_baseitem() const;
};

class BaseitemAllocationTable {
public:
    BaseitemAllocationTable(const BaseitemAllocationTable &) = delete;
    BaseitemAllocationTable(BaseitemAllocationTable &&) = delete;
    BaseitemAllocationTable operator=(const BaseitemAllocationTable &) = delete;
    BaseitemAllocationTable operator=(BaseitemAllocationTable &&) = delete;
    static BaseitemAllocationTable &get_instance();

    void initialize();
    std::vector<BaseitemAllocationEntry>::iterator begin();
    std::vector<BaseitemAllocationEntry>::const_iterator begin() const;
    std::vector<BaseitemAllocationEntry>::iterator end();
    std::vector<BaseitemAllocationEntry>::const_iterator end() const;
    size_t size() const;
    const BaseitemAllocationEntry &get_entry(int index) const;
    BaseitemAllocationEntry &get_entry(int index);

private:
    static BaseitemAllocationTable instance;
    BaseitemAllocationTable() = default;
    std::vector<BaseitemAllocationEntry> entries;
};

extern std::vector<BaseitemAllocationEntry> alloc_kind_table;
