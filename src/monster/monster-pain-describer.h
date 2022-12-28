#pragma once

#include "system/angband.h"

class PlayerType;
class MonsterEntity;

class MonsterPainDescriber {
public:
    MonsterPainDescriber(PlayerType *player_ptr, const MonsterEntity *m_ptr);
    MonsterPainDescriber(PlayerType *player_ptr, MONSTER_IDX m_idx);

    std::string describe(int dam);

private:
    PlayerType *player_ptr;
    const MonsterEntity *m_ptr;
};
