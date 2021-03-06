#include "mspell/mspell-attack-util.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

msa_type *initialize_msa_type(player_type *target_ptr, msa_type *msa_ptr, MONSTER_IDX m_idx)
{
    msa_ptr->m_idx = m_idx;
    msa_ptr->m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    msa_ptr->r_ptr = &r_info[msa_ptr->m_ptr->r_idx];
    msa_ptr->no_inate = randint0(100) >= (msa_ptr->r_ptr->freq_spell * 2);
    msa_ptr->f4 = msa_ptr->r_ptr->flags4;
    msa_ptr->f5 = msa_ptr->r_ptr->a_ability_flags1;
    msa_ptr->f6 = msa_ptr->r_ptr->a_ability_flags2;
    msa_ptr->x = target_ptr->x;
    msa_ptr->y = target_ptr->y;
    msa_ptr->x_br_lite = 0;
    msa_ptr->y_br_lite = 0;
    msa_ptr->do_spell = DO_SPELL_NONE;
    msa_ptr->num = 0;
    msa_ptr->thrown_spell = 0;
    return msa_ptr;
}
