#pragma once

#include "system/h-type.h"

/**
 * @brief 2次元平面上の座標を表す構造体
 */
template <typename T>
struct Point2D {
    T y{};
    T x{};
    constexpr Point2D(T y, T x)
        : y(y)
        , x(x)
    {
    }
};

//! ゲームの平面マップ上の座標位置を表す構造体
using Pos2D = Point2D<POSITION>;
