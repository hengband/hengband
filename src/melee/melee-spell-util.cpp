﻿#include "melee/melee-spell-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"

melee_spell_type *initialize_melee_spell_type(player_type *target_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx)
{
    ms_ptr->m_idx = m_idx;
    ms_ptr->y = 0;
    ms_ptr->x = 0;
    ms_ptr->target_idx = 0;
    ms_ptr->thrown_spell = 0;
    ms_ptr->dam = 0;
    ms_ptr->num = 0;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    ms_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    ms_ptr->t_ptr = NULL;
    ms_ptr->r_ptr = &r_info[ms_ptr->m_ptr->r_idx];
    ms_ptr->see_m = is_seen(target_ptr, ms_ptr->m_ptr);
    ms_ptr->maneable = player_has_los_bold(target_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx);
    ms_ptr->pet = is_pet(ms_ptr->m_ptr);
    ms_ptr->in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
        && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
    return ms_ptr;
}
