﻿#include "monster-floor/monster-death-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
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
static OBJECT_SUBTYPE_VALUE get_coin_type(MONRACE_IDX r_idx)
{
    switch (r_idx) {
    case MON_COPPER_COINS:
        return 2;
    case MON_SILVER_COINS:
        return 5;
    case MON_GOLD_COINS:
        return 10;
    case MON_MITHRIL_COINS:
    case MON_MITHRIL_GOLEM:
        return 16;
    case MON_ADAMANT_COINS:
        return 17;
    }

    return MON_PLAYER;
}

monster_death_type *initialize_monster_death_type(player_type *player_ptr, monster_death_type *md_ptr, MONSTER_IDX m_idx, bool drop_item)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    md_ptr->m_idx = m_idx;
    md_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    md_ptr->r_ptr = &r_info[md_ptr->m_ptr->r_idx];
    md_ptr->do_gold = (none_bits(md_ptr->r_ptr->flags1, (RF1_ONLY_ITEM | RF1_DROP_GOOD | RF1_DROP_GREAT)));
    md_ptr->do_item = (none_bits(md_ptr->r_ptr->flags1, RF1_ONLY_GOLD) || any_bits(md_ptr->r_ptr->flags1, (RF1_DROP_GOOD | RF1_DROP_GREAT)));
    md_ptr->cloned = any_bits(md_ptr->m_ptr->smart, SM_CLONED) ? TRUE : FALSE;
    md_ptr->force_coin = get_coin_type(md_ptr->m_ptr->r_idx);
    md_ptr->drop_chosen_item = drop_item && !md_ptr->cloned && !floor_ptr->inside_arena && !player_ptr->phase_out && !is_pet(md_ptr->m_ptr);
    return md_ptr;
}
