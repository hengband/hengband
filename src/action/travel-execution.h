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
    void set_goal(const Pos2D &pos);
    void reset_goal();
    bool is_started() const;
    bool is_ongoing() const;
    void stop();
    void step(PlayerType *player_ptr);

    int cost[MAX_HGT][MAX_WID]{};
    DIRECTION dir{}; /* Running direction */

private:
    std::optional<Pos2D> pos_goal; /* Target position */
    int run{}; /* Remaining grid number */
};

extern Travel travel;

void forget_travel_flow(const FloorType &floor);
