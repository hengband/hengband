#pragma once

#include "system/h-type.h"
#include <algorithm>
#include <concepts>
#include <type_traits>

/**
 * @brief 2次元平面上のベクトルを表すクラス
 */
template <typename T>
struct Vector2D {
    T y{};
    T x{};
    constexpr Vector2D(T y, T x)
        : y(y)
        , x(x)
    {
    }

    constexpr Vector2D &operator+=(const Vector2D &other)
    {
        this->y += other.y;
        this->x += other.x;
        return *this;
    }

    constexpr Vector2D &operator*=(T scalar)
    {
        this->y *= scalar;
        this->x *= scalar;
        return *this;
    }

    /*!
     * @brief 反転したベクトルを生成する
     * @return 反転ベクトル
     */
    constexpr Vector2D inverted() const
    {
        return Vector2D(-this->y, -this->x);
    }
};

/**
 * @brief 2次元平面上の座標を表すクラス
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

    constexpr Point2D &operator+=(const Vector2D<T> &vector)
    {
        this->y += vector.y;
        this->x += vector.x;
        return *this;
    }

    // 座標同士の加算は意味がないので明示的に削除しておく
    constexpr Point2D &operator+=(const Point2D &other) = delete;

    // 座標同士の減算の結果はVector2Dであり、自身から座標を引くことには意味がないので明示的に削除しておく
    constexpr Point2D &operator-=(const Point2D &other) = delete;

    constexpr Point2D centered(const Point2D &pos) const
    {
        return Point2D((this->y + pos.y) / 2, (this->x + pos.x) / 2);
    }
};

template <typename T>
constexpr bool operator==(const Point2D<T> &point1, const Point2D<T> &point2)
{
    return (point1.y == point2.y) && (point1.x == point2.x);
}

template <typename T>
constexpr bool operator!=(const Point2D<T> &point1, const Point2D<T> &point2)
{
    return !(point1 == point2);
}

template <typename T>
constexpr Point2D<T> operator+(const Point2D<T> &point, const Vector2D<T> &vector)
{
    auto result = point;
    result += vector;
    return result;
}

template <typename T>
constexpr Point2D<T> operator+(const Vector2D<T> &vector, const Point2D<T> &point)
{
    return operator+(point, vector);
}

template <typename T>
constexpr Vector2D<T> operator-(const Point2D<T> &point1, const Point2D<T> &point2)
{
    return { point1.y - point2.y, point1.x - point2.x };
}

template <typename T>
constexpr Vector2D<T> operator+(const Vector2D<T> &vector1, const Vector2D<T> &vector2)
{
    auto result = vector1;
    result += vector2;
    return result;
}

template <typename T>
constexpr Vector2D<T> operator*(const Vector2D<T> &vector, T scalar)
{
    auto result = vector;
    result *= scalar;
    return result;
}

// 座標同士の加算は意味がないので明示的に削除しておく
template <typename T>
constexpr Point2D<T> operator+(const Point2D<T> &, const Point2D<T> &) = delete;

/**
 * @brief 長方形を表すクラス (左上/右下の座標を所有する)
 */
template <std::integral T>
struct Rectangle2D {
    Point2D<T> top_left;
    Point2D<T> bottom_right;
    constexpr Rectangle2D(const Point2D<T> &pos1, const Point2D<T> &pos2)
        : top_left(std::min<T>(pos1.y, pos2.y), std::min<T>(pos1.x, pos2.x))
        , bottom_right(std::max<T>(pos1.y, pos2.y), std::max<T>(pos1.x, pos2.x))
    {
    }

    constexpr Rectangle2D(const Point2D<T> &center, const Vector2D<T> &vec)
        : Rectangle2D(center + vec, center + vec.inverted())
    {
    }

    constexpr Rectangle2D resized(T margin) const
    {
        const Vector2D<T> vec(margin, margin);
        return { this->top_left + vec.inverted(), this->bottom_right + vec };
    }

    template <std::invocable<Point2D<T>> F>
    void each_area(F &&f) const
    {
        for (auto y = this->top_left.y; y <= this->bottom_right.y; ++y) {
            for (auto x = this->top_left.x; x <= this->bottom_right.x; ++x) {
                f(Point2D<T>(y, x));
            }
        }
    }

    template <std::invocable<Point2D<T>> F>
    void each_edge(F &&f) const
    {
        for (auto y = this->top_left.y; y <= this->bottom_right.y; ++y) {
            f(Point2D<T>(y, top_left.x));
            f(Point2D<T>(y, bottom_right.x));
        }
        for (auto x = this->top_left.x; x <= this->bottom_right.x; ++x) {
            f(Point2D<T>(top_left.y, x));
            f(Point2D<T>(bottom_right.y, x));
        }
    }
};

//! ゲームの平面マップ上の座標位置を表す構造体
using Pos2D = Point2D<POSITION>;

using Pos2DVec = Vector2D<POSITION>;

using Rect2D = Rectangle2D<POSITION>;
