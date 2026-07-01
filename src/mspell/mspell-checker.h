#pragma once

#include "system/angband.h"

struct ProjectResult;
enum class MonsterAbilityType;
enum class AttributeType;

class MonsterEntity;
class PlayerType;
class AbstractAttribute;
bool clean_shot(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend);
bool summon_possible(PlayerType *player_ptr, POSITION y1, POSITION x1);
bool raise_possible(PlayerType *player_ptr, const MonsterEntity &monster);
bool spell_is_inate(MonsterAbilityType spell);
ProjectResult beam(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
ProjectResult bolt(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, AttributeType typ, int dam_hp, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
ProjectResult pointed(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
ProjectResult ball(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
ProjectResult breath(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
ProjectResult rocket(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, AttributeType typ, int dam_hp, POSITION rad, int target_type, std::shared_ptr<AbstractAttribute> attribute = nullptr);
