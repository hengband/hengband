#pragma once

#include "load/monster/monster-loader-base.h"

struct monster_type;
class PlayerType;
class MonsterLoader10 : public MonsterLoaderBase {
public:
    MonsterLoader10(PlayerType *player_ptr);
    void rd_monster(monster_type *m_ptr) override;

private:
    PlayerType *player_ptr;
    monster_type *m_ptr = nullptr;
};
