#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-resistance-mask.h"
#include "system/monster-race-definition.h"

/* The monster race arrays */
std::vector<monster_race> r_info;

int calc_monrace_power(monster_race *r_ptr)
{
    int ret = 0;
    int num_taisei = EnumClassFlagGroup<MonsterResistanceType>(r_ptr->resistance_flags & RFR_EFF_IMMUNE_ELEMENT_MASK).count();

    if (r_ptr->flags1 & RF1_FORCE_MAXHP)
        ret = r_ptr->hdice * r_ptr->hside * 2;
    else
        ret = r_ptr->hdice * (r_ptr->hside + 1);
    ret = ret * (100 + r_ptr->level) / 100;
    if (r_ptr->speed > 110)
        ret = ret * (r_ptr->speed * 2 - 110) / 100;
    if (r_ptr->speed < 110)
        ret = ret * (r_ptr->speed - 20) / 100;
    if (num_taisei > 2)
        ret = ret * (num_taisei * 2 + 5) / 10;
    else if (r_ptr->ability_flags.has(MonsterAbilityType::INVULNER))
        ret = ret * 4 / 3;
    else if (r_ptr->ability_flags.has(MonsterAbilityType::HEAL))
        ret = ret * 4 / 3;
    else if (r_ptr->ability_flags.has(MonsterAbilityType::DRAIN_MANA))
        ret = ret * 11 / 10;
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_25))
        ret = ret * 9 / 10;
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50))
        ret = ret * 9 / 10;
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL))
        ret *= 100000;
    if (r_ptr->arena_ratio)
        ret = ret * r_ptr->arena_ratio / 100;
    return ret;
}
