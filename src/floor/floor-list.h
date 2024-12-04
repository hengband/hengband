#pragma once

#include "system/floor/floor-info.h"

class FloorList final {
public:
    FloorList(const FloorList &) = delete;
    FloorList(FloorList &&) = delete;
    FloorList &operator=(const FloorList &) = delete;
    FloorList &operator=(FloorList &&) = delete;
    static FloorList &get_instance();

    FloorType &get_floor(int num);

private:
    static FloorList instance;
    FloorType floor{};
    FloorList() = default;
};
