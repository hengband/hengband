#include "melee/melee-spell-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

melee_spell_type *initialize_melee_spell_type(PlayerType *player_ptr, melee_spell_type *ms_ptr, MONSTER_IDX m_idx)
{
    ms_ptr->m_idx = m_idx;
    ms_ptr->y = 0;
    ms_ptr->x = 0;
    ms_ptr->target_idx = 0;
    ms_ptr->thrown_spell = MonsterAbilityType::MAX;
    ms_ptr->dam = 0;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    ms_ptr->m_ptr = &floor_ptr->m_list[m_idx];
    ms_ptr->t_ptr = nullptr;
    ms_ptr->r_ptr = &r_info[ms_ptr->m_ptr->r_idx];
    ms_ptr->see_m = is_seen(player_ptr, ms_ptr->m_ptr);
    ms_ptr->maneable = player_has_los_bold(player_ptr, ms_ptr->m_ptr->fy, ms_ptr->m_ptr->fx);
    ms_ptr->pet = is_pet(ms_ptr->m_ptr);
    ms_ptr->in_no_magic_dungeon = d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MAGIC) && floor_ptr->dun_level && (!inside_quest(floor_ptr->quest_number) || quest_type::is_fixed(floor_ptr->quest_number));
    return ms_ptr;
}
