#pragma once

#include <optional>
#include <vector>

class BaseitemKey;
class MonsterRaceInfo;
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
    const BaseitemKey &get_bi_key() const;
    MonsterRaceInfo &get_monrace();
    const MonsterRaceInfo &get_monrace() const;
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
