#pragma once

#include "system/h-type.h"
#include <algorithm>
#include <concepts>
#include <numeric>
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

    /*!
     * @brief 2点間の中点を求める
     * @param p1 点1
     * @param p2 点2
     * @return 点1と点2の中点
     * @note Tが整数型で結果が整数にならない場合、p1側に丸められる
     */
    static constexpr Point2D midpoint(const Point2D &p1, const Point2D &p2)
    {
        return Point2D(std::midpoint(p1.y, p2.y), std::midpoint(p1.x, p2.x));
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
    constexpr Rectangle2D(T y1, T x1, T y2, T x2)
        : top_left(std::min<T>(y1, y2), std::min<T>(x1, x2))
        , bottom_right(std::max<T>(y1, y2), std::max<T>(x1, x2))
    {
    }

    constexpr Rectangle2D(const Point2D<T> &pos1, const Point2D<T> &pos2)
        : Rectangle2D(pos1.y, pos1.x, pos2.y, pos2.x)
    {
    }

    constexpr Rectangle2D(const Point2D<T> &center, const Vector2D<T> &vec)
        : Rectangle2D(center + vec, center + vec.inverted())
    {
    }

    constexpr T width() const
    {
        return this->bottom_right.x - this->top_left.x + 1;
    }

    constexpr T height() const
    {
        return this->bottom_right.y - this->top_left.y + 1;
    }

    constexpr Point2D<T> center() const
    {
        return Point2D<T>::midpoint(this->top_left, this->bottom_right);
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
