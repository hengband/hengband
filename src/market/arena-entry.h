#pragma once

#include "system/baseitem-info.h"
#include <vector>

/*!
 * @brief 闘技場のモンスターエントリー構造体
 */
enum class MonsterRaceId : int16_t;
class ArenaMonsterEntry {
public:
    ArenaMonsterEntry(MonsterRaceId r_idx, const BaseitemKey &key)
        : r_idx(r_idx)
        , key(key)
    {
    }

    MonsterRaceId r_idx; /*!< 闘技場のモンスター種族ID(0ならば表彰式) / Monster (0 means victory prizing) */
    BaseitemKey key;
};

class ArenaEntryList {
public:
    ~ArenaEntryList() = default;
    ArenaEntryList(ArenaEntryList &&) = delete;
    ArenaEntryList(const ArenaEntryList &) = delete;
    ArenaEntryList &operator=(const ArenaEntryList &) = delete;
    ArenaEntryList &operator=(ArenaEntryList &&) = delete;
    static ArenaEntryList &get_instance();

    int get_max_entries() const;

private:
    ArenaEntryList() = default;

    static ArenaEntryList instance;
};

extern const std::vector<ArenaMonsterEntry> arena_info;
