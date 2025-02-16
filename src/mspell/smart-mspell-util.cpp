#include "mspell/smart-mspell-util.h"
#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"

msr_type::msr_type(PlayerType *player_ptr, short m_idx, const EnumClassFlagGroup<MonsterAbilityType> &ability_flags)
    : ability_flags(ability_flags)
{
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    this->r_ptr = &monster.get_monrace();
}

/*!
 * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
 * Internal probability routine
 * @param r_ptr モンスター種族の構造体参照ポインタ
 * @param prob 基本確率(%)
 * @return 適した選択を取るならばTRUEを返す。
 */
bool int_outof(const MonraceDefinition &monrace, int prob)
{
    if (monrace.behavior_flags.has_not(MonsterBehaviorType::SMART)) {
        prob = prob / 2;
    }

    return evaluate_percent(prob);
}
