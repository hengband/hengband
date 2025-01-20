#pragma once

#include "util/point-2d.h"
#include <cstdint>

// @todo PlayerType に依存するオブジェクトメソッドを追加予定.
class FloorType;
class MonsterEntity;
class MonraceDefinition;
class MonsterDeath {
public:
    MonsterDeath(FloorType &floor, short m_idx, bool drop_item);
    short m_idx;
    MonsterEntity *m_ptr;
    MonraceDefinition *r_ptr;
    bool do_gold;
    bool do_item;
    bool cloned;
    bool drop_chosen_item;
    int md_y = 0;
    int md_x = 0;
    uint32_t mo_mode = 0;

    Pos2D get_position() const;
};
