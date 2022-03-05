#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "system/monster-race-definition.h"
#include "util/probability-table.h"
#include <algorithm>
#include <vector>

/* The monster race arrays */
std::map<MonsterRaceId, monster_race> r_info;

int calc_monrace_power(monster_race *r_ptr)
{
    int ret = 0;
    int num_taisei = EnumClassFlagGroup<MonsterResistanceType>(r_ptr->resistance_flags & RFR_EFF_IMMUNE_ELEMENT_MASK).count();

    if (r_ptr->flags1 & RF1_FORCE_MAXHP) {
        ret = r_ptr->hdice * r_ptr->hside * 2;
    } else {
        ret = r_ptr->hdice * (r_ptr->hside + 1);
    }
    ret = ret * (100 + r_ptr->level) / 100;
    if (r_ptr->speed > 110) {
        ret = ret * (r_ptr->speed * 2 - 110) / 100;
    }
    if (r_ptr->speed < 110) {
        ret = ret * (r_ptr->speed - 20) / 100;
    }
    if (num_taisei > 2) {
        ret = ret * (num_taisei * 2 + 5) / 10;
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
        for (const auto &[r_idx, r_ref] : r_info) {
            if (MonsterRace(r_idx).is_valid()) {
                table.entry_item(r_idx, 1);
            }
        }
    }

    return table.pick_one_at_random();
}

/*!
 * @brief コンストラクタに渡された MonsterRaceId が正当なもの（実際に存在するモンスター種族IDである）かどうかを調べる
 * @details モンスター種族IDが r_info に実在するもの(MonsterRaceId::PLAYERは除く)であるかどうかの用途の他、
 * m_list 上の要素などの r_idx にMonsterRaceId::PLAYER を入れることで死亡扱いとして使われるのでその判定に使用する事もある
 * @return 正当なものであれば true、そうでなければ false
 */
bool MonsterRace::is_valid() const
{
    return this->r_idx != MonsterRaceId::PLAYER;
}
