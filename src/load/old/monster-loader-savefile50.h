#pragma once

#include "load/monster/monster-loader-base.h"

class MonsterEntity;
class PlayerType;
class MonsterLoader50 : public MonsterLoaderBase {
public:
    MonsterLoader50(PlayerType *player_ptr);
    void rd_monster(MonsterEntity *m_ptr) override;

private:
    PlayerType *player_ptr;
    MonsterEntity *m_ptr = nullptr;
};
