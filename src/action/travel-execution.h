#pragma once
/*!
 * @file travel-execution.h
 * @brief トラベル移動処理ヘッダ
 */

#include "floor/floor-base-definitions.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <optional>

class FloorType;
class PlayerType;

/*  A structure type for travel command  */
class Travel {
public:
    Travel() = default;

    const std::optional<Pos2D> &get_goal() const;
    void set_goal(const Pos2D &p_pos, const Pos2D &pos);
    void reset_goal();
    bool is_started() const;
    bool is_ongoing() const;
    void stop();
    void step(PlayerType *player_ptr);
    void update_flow(PlayerType *player_ptr);
    void forget_flow();

    std::array<std::array<int, MAX_WID>, MAX_HGT> costs{};

private:
    std::optional<Pos2D> pos_goal; /* Target position */
    int run{}; /* Remaining grid number */
    DIRECTION dir{}; /* Running direction */
};

extern Travel travel;
