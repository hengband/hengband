#include "monster-floor/monster-death-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターを倒した際の財宝svalを返す
 * @param r_idx 倒したモンスターの種族ID
 * @return 財宝のsval
 * @details
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 * Note the use of actual "monster names"
 */
static int get_coin_type(MonsterRaceId r_idx)
{
    switch (r_idx) {
    case MonsterRaceId::COPPER_COINS:
        return 2;
    case MonsterRaceId::SILVER_COINS:
        return 5;
    case MonsterRaceId::GOLD_COINS:
        return 10;
    case MonsterRaceId::MITHRIL_COINS:
    case MonsterRaceId::MITHRIL_GOLEM:
        return 16;
    case MonsterRaceId::ADAMANT_COINS:
        return 17;
    default:
        return 0;
    }
}

MonsterDeath::MonsterDeath(FloorType &floor, MONSTER_IDX m_idx, bool drop_item)
    : m_idx(m_idx)
    , m_ptr(&floor.m_list[m_idx])
{
    this->r_ptr = &this->m_ptr->get_monrace();
    this->do_gold = this->r_ptr->drop_flags.has_none_of({
        MonsterDropType::ONLY_ITEM,
        MonsterDropType::DROP_GOOD,
        MonsterDropType::DROP_GREAT,
    });
    this->do_item = this->r_ptr->drop_flags.has_not(MonsterDropType::ONLY_GOLD);
    this->do_item |= this->r_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });
    this->cloned = this->m_ptr->mflag2.has(MonsterConstantFlagType::CLONED);
    this->force_coin = get_coin_type(this->m_ptr->r_idx);
    this->drop_chosen_item = drop_item && !this->cloned && !floor.inside_arena && !AngbandSystem::get_instance().is_phase_out() && !this->m_ptr->is_pet();
}
