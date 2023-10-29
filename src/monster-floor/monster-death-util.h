#pragma once

#include "system/angband.h"

// @todo PlayerType に依存するオブジェクトメソッドを追加予定.
class FloorType;
class MonsterEntity;
class MonsterRaceInfo;
class MonsterDeath {
public:
    MonsterDeath(FloorType &floor, MONSTER_IDX m_idx, bool drop_item);
    MONSTER_IDX m_idx;
    MonsterEntity *m_ptr;
    MonsterRaceInfo *r_ptr;
    bool do_gold;
    bool do_item;
    bool cloned;
    int force_coin;
    bool drop_chosen_item;
    POSITION md_y = 0;
    POSITION md_x = 0;
    uint32_t mo_mode = 0;
};
