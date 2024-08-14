#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct building_type {
    char name[20]{}; /* proprietor name */
    char owner_name[20]{}; /* proprietor name */
    char owner_race[20]{}; /* proprietor race */

    char act_names[8][30]{}; /* action names */
    int member_costs[8]{}; /* Costs for class members of building */
    int other_costs[8]{}; /* Costs for nonguild members */
    char letters[8]{}; /* action letters */
    short actions[8]{}; /*!< 町の施設処理における行動ID */
    short action_restr[8]{}; /*!< 町の施設処理の規制処理ID */

    std::vector<short> member_class{}; /* which classes are part of guild */
    std::vector<short> member_race{}; /* which races are part of guild */
    std::vector<short> member_realm{}; /* 店主ごとの魔法領域 / which realms are part of guild */
};

constexpr auto MAX_BUILDINGS = 32; /*!< 施設の種類最大数 / Number of buildings */
extern std::array<building_type, MAX_BUILDINGS> buildings;

enum class MonsterRaceId : short;
class MonsterRaceInfo;
class MeleeGladiator {
public:
    MeleeGladiator() = default;
    MeleeGladiator(MonsterRaceId monrace_id, uint32_t odds);
    MonsterRaceId monrace_id{};
    uint32_t odds = 0;

    const MonsterRaceInfo &get_monrace() const;
};

extern int battle_odds;

//!< モンスター闘技場定義.
constexpr auto NUM_GLADIATORS = 4;
class PlayerType; //!< @todo 暫定、後で消す.
class MeleeArena {
public:
    ~MeleeArena() = default;
    MeleeArena(MeleeArena &&) = delete;
    MeleeArena(const MeleeArena &) = delete;
    MeleeArena &operator=(const MeleeArena &) = delete;
    MeleeArena &operator=(MeleeArena &&) = delete;
    static MeleeArena &get_instance();

    int bet_number = 0;
    int get_wager() const;
    void set_wager(int value);
    MeleeGladiator &get_gladiator(int n);
    const MeleeGladiator &get_gladiator(int n) const;
    void set_gladiator(int n, const MeleeGladiator &gladiator);
    const std::array<MeleeGladiator, NUM_GLADIATORS> &get_gladiators() const; //!< @detail セーブデータへの書き込みにしか使わないこと.
    std::vector<std::string> build_gladiators_names() const; //!< @detail 要素数は常にNUM_GLADIATORSと同じ.
    void update_gladiators(PlayerType *player_ptr);

private:
    MeleeArena() = default;
    static MeleeArena instance;

    int wager = 0; //!< @detail 引き分け時の払い戻しに必要.
    std::array<MeleeGladiator, NUM_GLADIATORS> gladiators{};

    int decide_max_level() const;
    std::pair<int, bool> set_gladiators(PlayerType *player_ptr, int mon_level);
    MonsterRaceId search_gladiator(PlayerType *player_ptr, int mon_level, int num_gladiator) const;
    int matches_gladiator(MonsterRaceId monrace_id, int current_num) const;
    std::pair<int, int> set_odds(int current_total, bool is_applicable);
};
