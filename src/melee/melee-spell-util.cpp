#include "melee/melee-spell-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"

melee_spell_type::melee_spell_type(PlayerType *player_ptr, MONSTER_IDX m_idx)
    : m_idx(m_idx)
    , thrown_spell(MonsterAbilityType::MAX)
{
    auto &floor = *player_ptr->current_floor_ptr;
    this->m_ptr = &floor.m_list[m_idx];
    this->t_ptr = nullptr;
    this->r_ptr = &this->m_ptr->get_monrace();
    this->see_m = is_seen(player_ptr, this->m_ptr);
    this->maneable = floor.has_los({ this->m_ptr->fy, this->m_ptr->fx });
    this->pet = this->m_ptr->is_pet();
    const auto &dungeon = floor.get_dungeon_definition();
    const auto is_in_dungeon = floor.is_in_dungeon();
    const auto is_in_random_quest = floor.is_in_quest() && !QuestType::is_fixed(floor.quest_number);
    this->in_no_magic_dungeon = dungeon.flags.has(DungeonFeatureType::NO_MAGIC) && is_in_dungeon && !is_in_random_quest;
}
