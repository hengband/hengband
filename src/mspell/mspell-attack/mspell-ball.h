#pragma once

#include "system/angband.h"

#include "mspell/mspell-attack/abstract-mspell.h"

struct MonsterSpellResult;

class MSpellData;
class MSpellBall : public AbstractMSpellAttack {
public:
    MSpellBall(PlayerType *player_ptr, MONSTER_IDX m_idx, MonsterAbilityType ability, POSITION rad, int target_type);
    MSpellBall(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, MonsterAbilityType ability, POSITION rad, int target_type);
    ~MSpellBall() = default;
    MSpellBall(const MSpellBall &) = delete;
    MSpellBall(MSpellBall &&) = default;
    void operator=(const MSpellBall &) = delete;
    MSpellBall &operator=(MSpellBall &&) = default;
};

class PlayerType;
