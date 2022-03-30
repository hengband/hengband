#pragma once

#include "system/angband.h"

#include "mspell/mspell-attack/abstract-mspell.h"

struct MonsterSpellResult;

class MSpellBolt : public AbstractMSpellAttack {
public:
    MSpellBolt(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, int target_type);
    MSpellBolt(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, int target_type);
    ~MSpellBolt() = default;
    MSpellBolt(const MSpellBolt &) = delete;
    MSpellBolt(MSpellBolt &&) = default;
    void operator=(const MSpellBolt &) = delete;
    MSpellBolt &operator=(MSpellBolt &&) = default;
};

class PlayerType;
