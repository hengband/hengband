#pragma once

#include "system/angband.h"

struct ProjectResult;
enum class MonsterAbilityType;
enum class AttributeType;

class MonsterEntity;
class PlayerType;
bool clean_shot(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(PlayerType *player_ptr, POSITION y1, POSITION x1);
bool raise_possible(PlayerType *player_ptr, MonsterEntity *m_ptr);
bool spell_is_inate(MonsterAbilityType spell);
ProjectResult beam(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type);
ProjectResult bolt(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type);
ProjectResult pointed(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, int target_type);
ProjectResult ball(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
ProjectResult breath(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
ProjectResult rocket(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type);
