#pragma once

#include "util/point-2d.h"
#include <utility>
#include <vector>

/* Minimum & maximum town size */
#define MIN_TOWN_WID ((MAX_WID / 3) / 2)
#define MIN_TOWN_HGT ((MAX_HGT / 3) / 2)
#define MAX_TOWN_WID ((MAX_WID / 3) * 2 / 3)
#define MAX_TOWN_HGT ((MAX_HGT / 3) * 2 / 3)

class UndergroundBuilding {
public:
    UndergroundBuilding();
    Pos2D pick_door_direction() const;
    void set_area(int height, int width, int max_height, int max_width);
    bool is_area_used(const std::vector<std::vector<bool>> &ugarcade_used) const;
    void reserve_area(std::vector<std::vector<bool>> &ugarcade_used) const;
    std::pair<Pos2D, Pos2D> get_room_positions(const Pos2D &pos_ug) const;
    std::pair<Pos2D, Pos2D> get_inner_room_positions(const Pos2D &pos_ug) const;

private:
    Pos2D north_west; // 地下店舗左上座標.
    Pos2D south_east; // 地下店舗右下座標.
};

struct dun_data_type;
class PlayerType;
bool build_type16(PlayerType *player_ptr, dun_data_type *dd_ptr);
