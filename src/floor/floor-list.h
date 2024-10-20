#pragma once

#include "system/floor-type-definition.h"

class FloorList final {
public:
    FloorList(const FloorList &) = delete;
    FloorList(FloorList &&) = delete;
    FloorList &operator=(const FloorList &) = delete;
    FloorList &operator=(FloorList &&) = delete;
    static FloorList &get_instance();

    FloorType &get_floor(int num = 0); // 現状は何を指定しても一緒なため、デフォルトで0を指定する。FloorType 参照の引数への置き換え等が完了次第、代入は消すこと

private:
    static FloorList instance;
    FloorType floor{};
    FloorList() = default;
};
