#include "melee/melee-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "melee/melee-switcher.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"

mam_type *initialize_mam_type(PlayerType *player_ptr, mam_type *mam_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
    mam_ptr->attribute = BlowEffectType::NONE;
    mam_ptr->m_idx = m_idx;
    mam_ptr->t_idx = t_idx;
    mam_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    mam_ptr->t_ptr = &player_ptr->current_floor_ptr->m_list[t_idx];
    mam_ptr->damage = 0;
    mam_ptr->see_m = is_seen(player_ptr, mam_ptr->m_ptr);
    mam_ptr->see_t = is_seen(player_ptr, mam_ptr->t_ptr);
    mam_ptr->see_either = mam_ptr->see_m || mam_ptr->see_t;
    mam_ptr->y_saver = mam_ptr->t_ptr->fy;
    mam_ptr->x_saver = mam_ptr->t_ptr->fx;
    mam_ptr->explode = false;
    mam_ptr->touched = false;

    auto *r_ptr = &monraces_info[mam_ptr->m_ptr->r_idx];
    monster_race *tr_ptr = &monraces_info[mam_ptr->t_ptr->r_idx];
    mam_ptr->ac = tr_ptr->ac;
    mam_ptr->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    mam_ptr->blinked = false;
    mam_ptr->do_silly_attack = (one_in_(2) && player_ptr->effects()->hallucination()->is_hallucinated());
    mam_ptr->power = 0;
    mam_ptr->obvious = false;
    mam_ptr->known = (mam_ptr->m_ptr->cdis <= MAX_SIGHT) || (mam_ptr->t_ptr->cdis <= MAX_SIGHT);
    mam_ptr->fear = false;
    mam_ptr->dead = false;
    return mam_ptr;
}
