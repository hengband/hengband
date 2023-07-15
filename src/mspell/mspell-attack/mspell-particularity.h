#pragma once

#include "mspell/mspell-attack/abstract-mspell.h"
#include "system/angband.h"

struct MonsterSpellResult;
class PlayerType;
class MSpellData;

class MSpellAttackOther : public AbstractMSpellAttack {
public:
    MSpellAttackOther(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    MSpellAttackOther(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, MSpellData data, int target_type, std::function<ProjectResult(POSITION, POSITION, int, AttributeType)> fire);
    ~MSpellAttackOther() = default;
    MSpellAttackOther(const MSpellAttackOther &) = delete;
    MSpellAttackOther(MSpellAttackOther &&) = default;
    void operator=(const MSpellAttackOther &) = delete;
    MSpellAttackOther &operator=(MSpellAttackOther &&) = default;
};
MonsterSpellResult spell_RF4_ROCKET(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_HAND_DOOM(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
MonsterSpellResult spell_RF6_PSY_SPEAR(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type);
