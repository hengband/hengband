#pragma once

#include <array>
#include <cstdint>
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
extern MonsterRaceId battle_mon_list[4];
extern uint32_t mon_odds[4];
extern int battle_odds;
extern int wager_melee;
extern int bet_number;

//!< モンスター闘技場定義.
class MeleeArena {
public:
    ~MeleeArena() = default;
    MeleeArena(MeleeArena &&) = delete;
    MeleeArena(const MeleeArena &) = delete;
    MeleeArena &operator=(const MeleeArena &) = delete;
    MeleeArena &operator=(MeleeArena &&) = delete;
    static MeleeArena &get_instance();

private:
    MeleeArena() = default;
    static MeleeArena instance;
};
