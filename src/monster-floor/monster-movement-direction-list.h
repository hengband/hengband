#pragma once

#include "system/angband.h"
#include <span>

class Direction;

/*!
 * @brief モンスターの移動方向のリストを管理するクラス
 */
class MonsterMovementDirectionList {
public:
    explicit MonsterMovementDirectionList(MONSTER_IDX m_idx);
    static MonsterMovementDirectionList random_move(MONSTER_IDX m_idx);

    void add_movement_direction(const Direction &dir);
    void add_movement_directions(std::initializer_list<Direction> dirs);
    std::span<const Direction> get_movement_directions() const;
    MONSTER_IDX get_m_idx() const;

private:
    std::vector<Direction> movement_directions; ///< 移動方向のリスト
    MONSTER_IDX m_idx; ///< モンスターの参照インデックス
};
