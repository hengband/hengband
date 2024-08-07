#include "system/building-type-definition.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include <numeric>

std::array<building_type, MAX_BUILDINGS> buildings;
int battle_odds;
int wager_melee;
int bet_number;

MeleeGladiator::MeleeGladiator(MonsterRaceId monrace_id, uint32_t odds)
    : monrace_id(monrace_id)
    , odds(odds)
{
}

const MonsterRaceInfo &MeleeGladiator::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

MeleeArena MeleeArena::instance{};

MeleeArena &MeleeArena::get_instance()
{
    return instance;
}

MeleeGladiator &MeleeArena::get_gladiator(int n)
{
    return this->gladiators.at(n);
}

const MeleeGladiator &MeleeArena::get_gladiator(int n) const
{
    return this->gladiators.at(n);
}

void MeleeArena::set_gladiator(int n, const MeleeGladiator &gladiator)
{
    this->gladiators[n] = gladiator;
}

const std::array<MeleeGladiator, NUM_GLADIATORS> &MeleeArena::get_gladiators() const
{
    return this->gladiators;
}

void MeleeArena::update_gladiators(PlayerType *player_ptr, int mon_level)
{
    while (true) {
        auto [total, is_applicable] = this->set_gladiators(player_ptr, mon_level);
        const auto &[count, new_total] = this->set_odds(total, is_applicable);
        total = new_total;
        if (count == NUM_GLADIATORS) {
            break;
        }
    }
}

std::pair<int, bool> MeleeArena::set_gladiators(PlayerType *player_ptr, int mon_level)
{
    auto total = 0;
    auto is_applicable = false;
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        auto &gladiator = this->get_gladiator(i);
        gladiator.monrace_id = this->search_gladiator(player_ptr, mon_level, i);
        if (gladiator.get_monrace().level < 45) {
            is_applicable = true;
        }
    }

    return { total, is_applicable };
}

/*!
 * @brief モンスター闘技場に出場する4種のモンスターを強度で比較しオッズを決める
 * @param current_total 計算済のモンスター数
 * @param is_applicable モンスターのレベルが基準値未満か否か (基準値以上のモンスターはブレが激しすぎる)
 * @return このメソッドで計算したモンスター数と累積計算済モンスター数のペア
 */
std::pair<int, int> MeleeArena::set_odds(int current_total, bool is_applicable)
{
    const auto &monraces = MonraceList::get_instance();
    std::array<int, NUM_GLADIATORS> powers;
    std::transform(std::begin(this->gladiators), std::end(this->gladiators), std::begin(powers),
        [&monraces](const auto &gladiator) { return monraces.get_monrace(gladiator.monrace_id).calc_power(); });
    const auto total = current_total + std::reduce(std::begin(powers), std::end(powers));
    auto count = 0;
    for (auto &power : powers) {
        if (power <= 0) {
            break;
        }

        power = total * 60 / power;
        if (is_applicable && ((power < 160) || power > 1500)) {
            break;
        }

        if ((power < 160) && randint0(20)) {
            break;
        }

        if (power < 101) {
            power = 100 + randint1(5);
        }

        this->gladiators[count].odds = power;
        count++;
    }

    return { count, total };
}

MonsterRaceId MeleeArena::search_gladiator(PlayerType *player_ptr, int mon_level, int num_gladiator) const
{
    const auto &monraces = MonraceList::get_instance();
    MonsterRaceId monrace_id;
    while (true) {
        get_mon_num_prep(player_ptr, monster_can_entry_arena, nullptr);
        monrace_id = get_mon_num(player_ptr, 0, mon_level, PM_ARENA);
        if (!MonraceList::is_valid(monrace_id)) {
            continue;
        }

        const auto &monrace = monraces.get_monrace(monrace_id);
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE) || monrace.population_flags.has(MonsterPopulationType::ONLY_ONE)) {
            if ((monrace.level + 10) > mon_level) {
                continue;
            }
        }

        const auto count = this->matches_gladiator(monrace_id, num_gladiator);
        if (count < num_gladiator) {
            continue;
        }

        return monrace_id;
    }
}

int MeleeArena::matches_gladiator(MonsterRaceId monrace_id, int current_num) const
{
    for (auto count = 0; count < current_num; count++) {
        if (monrace_id == this->get_gladiator(count).monrace_id) {
            return count;
        }
    }

    return NUM_GLADIATORS;
}
