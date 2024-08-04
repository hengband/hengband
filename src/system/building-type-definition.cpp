#include "system/building-type-definition.h"
#include "system/monster-race-info.h"
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
