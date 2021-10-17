#pragma once

#include "load/monster/monster-loader-base.h"

struct monster_type;
struct player_type;
class MonsterLoader10 : public MonsterLoaderBase {
public:
    MonsterLoader10(player_type *player_ptr);
    void rd_monster(monster_type *m_ptr) override;

private:
    player_type *player_ptr;
    monster_type *m_ptr = nullptr;
};
