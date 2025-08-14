#include "monster-floor/monster-death-util.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

MonsterDeath::MonsterDeath(FloorType &floor, short m_idx, bool drop_item)
    : m_idx(m_idx)
    , m_ptr(&floor.m_list[m_idx])
{
    this->r_ptr = &this->m_ptr->get_monrace();
    this->ap_r_ptr = this->r_ptr;
    this->do_gold = this->r_ptr->drop_flags.has_none_of({
        MonsterDropType::ONLY_ITEM,
        MonsterDropType::DROP_GOOD,
        MonsterDropType::DROP_GREAT,
    });
    this->do_item = this->r_ptr->drop_flags.has_not(MonsterDropType::ONLY_GOLD);
    this->do_item |= this->r_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });
    this->cloned = this->m_ptr->mflag2.has(MonsterConstantFlagType::CLONED);
    this->is_chameleon = this->m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON);
    this->drop_chosen_item = drop_item && !this->cloned && !this->is_chameleon && !floor.inside_arena && !AngbandSystem::get_instance().is_phase_out() && !this->m_ptr->is_pet();
}

Pos2D MonsterDeath::get_position() const
{
    return { this->md_y, this->md_x };
}
