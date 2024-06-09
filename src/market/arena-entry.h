#pragma once

#include "system/baseitem-info.h"
#include "system/h-type.h"
#include <optional>
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
    int get_current_entry() const;
    std::optional<int> get_defeated_entry() const;
    bool is_player_victor() const;
    bool is_player_true_victor() const;
    void increment_entry();
    void reset_entry();
    void set_defeated_entry();
    void load_current_entry(int entry);
    void load_defeated_entry(int entry);

private:
    ArenaEntryList() = default;

    static ArenaEntryList instance;
    int current_entry = 0; //!< 現在の対戦相手.
    std::optional<int> defeated_entry; //!< 負けた相手. 無敗ならnullopt. v1.5.0.1以前の敗北済セーブデータは0固定.
};

extern const std::vector<ArenaMonsterEntry> arena_info;
