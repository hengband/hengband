#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "system/monster-race-info.h"
#include "util/probability-table.h"
#include "world/world.h"
#include <algorithm>
#include <vector>

/* The monster race arrays */
std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

MonsterRace::MonsterRace(MonsterRaceId r_idx)
    : r_idx(r_idx)
{
}

/*!
 * @brief (MonsterRaceId::PLAYERを除く)実在するすべてのモンスター種族IDから等確率で1つ選択する
 *
 * @return 選択したモンスター種族ID
 */
MonsterRaceId MonsterRace::pick_one_at_random()
{
    static ProbabilityTable<MonsterRaceId> table;

    if (table.empty()) {
        for (const auto &[monrace_id, monrace] : monraces_info) {
            if (monrace.is_valid()) {
                table.entry_item(monrace_id, 1);
            }
        }
    }

    return table.pick_one_at_random();
}

/*!
 * @brief モンスター種族の総合的な強さを計算する。
 * @details 現在はモンスター闘技場でのモンスターの強さの総合的な評価にのみ使用されている。
 * @return 計算した結果のモンスター種族の総合的な強さの値を返す。
 */
int MonsterRace::calc_power() const
{
    int ret = 0;
    const auto *r_ptr = &monraces_info[this->r_idx];
    auto num_resistances = EnumClassFlagGroup<MonsterResistanceType>(r_ptr->resistance_flags & RFR_EFF_IMMUNE_ELEMENT_MASK).count();

    if (r_ptr->misc_flags.has(MonsterMiscType::FORCE_MAXHP)) {
        ret = r_ptr->hdice * r_ptr->hside * 2;
    } else {
        ret = r_ptr->hdice * (r_ptr->hside + 1);
    }
    ret = ret * (100 + r_ptr->level) / 100;
    if (r_ptr->speed > STANDARD_SPEED) {
        ret = ret * (r_ptr->speed * 2 - 110) / 100;
    }
    if (r_ptr->speed < STANDARD_SPEED) {
        ret = ret * (r_ptr->speed - 20) / 100;
    }
    if (num_resistances > 2) {
        ret = ret * (num_resistances * 2 + 5) / 10;
    } else if (r_ptr->ability_flags.has(MonsterAbilityType::INVULNER)) {
        ret = ret * 4 / 3;
    } else if (r_ptr->ability_flags.has(MonsterAbilityType::HEAL)) {
        ret = ret * 4 / 3;
    } else if (r_ptr->ability_flags.has(MonsterAbilityType::DRAIN_MANA)) {
        ret = ret * 11 / 10;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25)) {
        ret = ret * 9 / 10;
    }
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50)) {
        ret = ret * 9 / 10;
    }
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        ret *= 100000;
    }
    if (r_ptr->arena_ratio) {
        ret = ret * r_ptr->arena_ratio / 100;
    }
    return ret;
}
