#include "melee/melee-util.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"

mam_type *initialize_mam_type(player_type *subject_ptr, mam_type *mam_ptr, MONRACE_IDX m_idx, MONRACE_IDX t_idx)
{
    mam_ptr->effect_type = 0;
    mam_ptr->m_idx = m_idx;
    mam_ptr->t_idx = t_idx;
    mam_ptr->m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
    mam_ptr->t_ptr = &subject_ptr->current_floor_ptr->m_list[t_idx];
    mam_ptr->damage = 0;
    mam_ptr->see_m = is_seen(subject_ptr, mam_ptr->m_ptr);
    mam_ptr->see_t = is_seen(subject_ptr, mam_ptr->t_ptr);
    mam_ptr->see_either = mam_ptr->see_m || mam_ptr->see_t;
    mam_ptr->y_saver = mam_ptr->t_ptr->fy;
    mam_ptr->x_saver = mam_ptr->t_ptr->fx;
    mam_ptr->explode = FALSE;
    mam_ptr->touched = FALSE;

    monster_race *r_ptr = &r_info[mam_ptr->m_ptr->r_idx];
    monster_race *tr_ptr = &r_info[mam_ptr->t_ptr->r_idx];
    mam_ptr->ac = tr_ptr->ac;
    mam_ptr->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    mam_ptr->blinked = FALSE;
    mam_ptr->do_silly_attack = (one_in_(2) && subject_ptr->image);
    mam_ptr->power = 0;
    mam_ptr->obvious = FALSE;
    mam_ptr->known = (mam_ptr->m_ptr->cdis <= MAX_SIGHT) || (mam_ptr->t_ptr->cdis <= MAX_SIGHT);
    mam_ptr->fear = FALSE;
    mam_ptr->dead = FALSE;
    return mam_ptr;
}
