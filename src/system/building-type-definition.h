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

enum class MonraceId : short;
class MonraceDefinition;
class MeleeGladiator {
public:
    MeleeGladiator() = default;
    MeleeGladiator(MonraceId monrace_id, uint32_t odds);
    MonraceId monrace_id{};
    uint32_t odds = 0;

    const MonraceDefinition &get_monrace() const;
};

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

    bool matches_bet_number(int value) const;
    void set_bet_number(int value);
    void set_wager(int value);
    int get_payback(bool is_draw = false) const;
    MeleeGladiator &get_gladiator(int n);
    const MeleeGladiator &get_gladiator(int n) const;
    void set_gladiator(int n, const MeleeGladiator &gladiator);
    const std::array<MeleeGladiator, NUM_GLADIATORS> &get_gladiators() const; //!< @detail セーブデータへの書き込みにしか使わないこと.
    std::vector<std::string> build_gladiators_names() const; //!< @detail 要素数は常にNUM_GLADIATORSと同じ.
    void update_gladiators(PlayerType *player_ptr);

private:
    MeleeArena() = default;
    static MeleeArena instance;

    int bet_number = 0;
    int wager = 0;
    std::array<MeleeGladiator, NUM_GLADIATORS> gladiators{};

    int decide_max_level() const;
    std::pair<int, bool> set_gladiators(PlayerType *player_ptr, int mon_level);
    MonraceId search_gladiator(PlayerType *player_ptr, int mon_level, int num_gladiator) const;
    int matches_gladiator(MonraceId monrace_id, int current_num) const;
    std::pair<int, int> set_odds(int current_total, bool is_applicable);
};
