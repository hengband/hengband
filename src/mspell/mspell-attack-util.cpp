#include "mspell/mspell-attack-util.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

msa_type::msa_type(PlayerType *player_ptr, MONSTER_IDX m_idx)
    : m_idx(m_idx)
    , m_ptr(&player_ptr->current_floor_ptr->m_list[m_idx])
    , x(player_ptr->x)
    , y(player_ptr->y)
    , do_spell(DO_SPELL_NONE)
    , thrown_spell(MonsterAbilityType::MAX)
{
    this->r_ptr = &monraces_info[this->m_ptr->r_idx];
    this->no_inate = randint0(100) >= (this->r_ptr->freq_spell * 2);
    this->ability_flags = this->r_ptr->ability_flags;
}
