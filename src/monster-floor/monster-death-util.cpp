#include "monster-floor/monster-death-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
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
static OBJECT_SUBTYPE_VALUE get_coin_type(MonsterRaceId r_idx)
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

monster_death_type *initialize_monster_death_type(PlayerType *player_ptr, monster_death_type *md_ptr, MONSTER_IDX m_idx, bool drop_item)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    md_ptr->m_idx = m_idx;
    md_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    md_ptr->r_ptr = &monraces_info[md_ptr->m_ptr->r_idx];
    md_ptr->do_gold = (md_ptr->r_ptr->drop_flags.has_none_of({ MonsterDropType::ONLY_ITEM, MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT }));
    md_ptr->do_item = (md_ptr->r_ptr->drop_flags.has_not(MonsterDropType::ONLY_GOLD) || md_ptr->r_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT }));
    md_ptr->cloned = md_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::CLONED);
    md_ptr->force_coin = get_coin_type(md_ptr->m_ptr->r_idx);
    md_ptr->drop_chosen_item = drop_item && !md_ptr->cloned && !floor_ptr->inside_arena && !player_ptr->phase_out && !md_ptr->m_ptr->is_pet();
    return md_ptr;
}
