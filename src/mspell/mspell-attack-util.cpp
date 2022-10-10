#include "mspell/mspell-attack-util.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

msa_type *initialize_msa_type(PlayerType *player_ptr, msa_type *msa_ptr, MONSTER_IDX m_idx)
{
    msa_ptr->m_idx = m_idx;
    msa_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    msa_ptr->r_ptr = &monraces_info[msa_ptr->m_ptr->r_idx];
    msa_ptr->no_inate = randint0(100) >= (msa_ptr->r_ptr->freq_spell * 2);
    msa_ptr->ability_flags = msa_ptr->r_ptr->ability_flags;
    msa_ptr->x = player_ptr->x;
    msa_ptr->y = player_ptr->y;
    msa_ptr->x_br_lite = 0;
    msa_ptr->y_br_lite = 0;
    msa_ptr->do_spell = DO_SPELL_NONE;
    msa_ptr->dam = 0;
    msa_ptr->thrown_spell = MonsterAbilityType::MAX;
    return msa_ptr;
}
