#include "system/building-type-definition.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "object/object-value.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/services/dungeon-service.h"
#include <algorithm>
#include <limits>
#include <numeric>

std::array<building_type, MAX_BUILDINGS> buildings;

MeleeGladiator::MeleeGladiator(MonraceId monrace_id, uint32_t odds)
    : monrace_id(monrace_id)
    , odds(odds)
{
}

const MonraceDefinition &MeleeGladiator::get_monrace() const
{
    return MonraceList::get_instance().get_monrace(this->monrace_id);
}

MeleeArena MeleeArena::instance{};

MeleeArena &MeleeArena::get_instance()
{
    return instance;
}

bool MeleeArena::matches_bet_number(int value) const
{
    return this->bet_number == value;
}

void MeleeArena::set_bet_number(int value)
{
    this->bet_number = value;
}

void MeleeArena::set_wager(int value)
{
    this->wager = value;
}

/*!
 * @brief 賭けに勝った時の払い戻しを計算する
 * @param wager 賭け金
 * @param odds 賭けたモンスターのオッズ (100で1倍)
 * @return 払い戻し (賭け金より1以上多く、PRICE の上限以下)
 * @details 賭け金とオッズの積は int ではあふれうるので int64_t で計算し、ほかの金額と同じく PRICE の範囲に抑える。
 */
int MeleeArena::calc_payback(int wager, uint32_t odds)
{
    const auto payback = int64_t{ wager } * odds / 100;
    return clamp_price(std::max(int64_t{ wager } + 1, payback));
}

int MeleeArena::get_payback(bool is_draw) const
{
    if (is_draw) {
        return this->wager;
    }

    return calc_payback(this->wager, this->get_gladiator(this->bet_number).odds);
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

std::vector<std::string> MeleeArena::build_gladiators_names() const
{
    std::vector<std::string> names;
    for (const auto &gladiator : this->gladiators) {
        const auto &monrace = gladiator.get_monrace();
        std::stringstream ss;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            ss << _(monrace.name, "Fake ") << _("もどき", monrace.name);
        } else {
            ss << monrace.name << _("      ", "");
        }

        names.push_back(ss.str());
    }

    return names;
}

/*!
 * @brief モンスター闘技場に参加するモンスターを更新する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void MeleeArena::update_gladiators(PlayerType *player_ptr)
{
    const auto mon_level = DungeonService::decide_gradiator_level();
    while (true) {
        auto [total, is_applicable] = this->set_gladiators(player_ptr, mon_level);
        const auto &[count, new_total] = this->set_odds(total, is_applicable);
        total = new_total;
        if (count == NUM_GLADIATORS) {
            break;
        }
    }
}

std::pair<int64_t, bool> MeleeArena::set_gladiators(PlayerType *player_ptr, int mon_level)
{
    int64_t total = 0;
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
 * @details 強さとその和は int ではあふれうるので int64_t で計算する。
 * オッズはセーブで uint32_t として書き、表示や払い戻しの計算でも使うので、int の上限で抑えてから保存する
 * (修正前もオッズは int を超えなかったので、あふれなかった値は変わらない)。
 */
std::pair<int, int64_t> MeleeArena::set_odds(int64_t current_total, bool is_applicable)
{
    const auto &monraces = MonraceList::get_instance();
    std::array<int64_t, NUM_GLADIATORS> powers;
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

        this->gladiators[count].odds = static_cast<uint32_t>(std::min<int64_t>(power, std::numeric_limits<int>::max()));
        count++;
    }

    return { count, total };
}

MonraceId MeleeArena::search_gladiator(PlayerType *player_ptr, int mon_level, int num_gladiator) const
{
    const auto &monraces = MonraceList::get_instance();
    MonraceId monrace_id;
    while (true) {
        get_mon_num_prep_enum(player_ptr, MonraceHook::ARENA);
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

int MeleeArena::matches_gladiator(MonraceId monrace_id, int current_num) const
{
    for (auto count = 0; count < current_num; count++) {
        if (monrace_id == this->get_gladiator(count).monrace_id) {
            return count;
        }
    }

    return NUM_GLADIATORS;
}
