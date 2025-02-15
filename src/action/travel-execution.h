#pragma once
/*!
 * @file travel-execution.h
 * @brief トラベル移動処理ヘッダ
 */

#include "floor/floor-base-definitions.h"
#include "floor/geometry.h"
#include "util/point-2d.h"
#include <optional>

enum class TravelState {
    STOP, ///< トラベル停止中
    STANDBY_TO_EXECUTE, ///< トラベル実行開始待機中
    EXECUTING, ///< トラベル実行中
};

class FloorType;
class PlayerType;

/*  A structure type for travel command  */
class Travel {
public:
    Travel(const Travel &) = delete;
    Travel &operator=(const Travel &) = delete;
    Travel(Travel &&) = delete;
    Travel &operator=(Travel &&) = delete;

    static Travel &get_instance();
    static bool can_travel_to(const FloorType &floor, const Pos2D &pos);

    const std::optional<Pos2D> &get_goal() const;
    void set_goal(PlayerType *player_ptr, const Pos2D &pos);
    void reset_goal();
    bool is_ongoing() const;
    void stop();
    void step(PlayerType *player_ptr);
    int get_cost(const Pos2D &pos) const;

private:
    Travel() = default;

    void update_flow(PlayerType *player_ptr);
    void forget_flow();

    std::optional<Pos2D> pos_goal; /* Target position */
    TravelState state = TravelState::STOP;
    Direction dir = Direction::none(); /* Running direction */
    std::array<std::array<int, MAX_WID>, MAX_HGT> costs{};
};
