#pragma once

#include <concepts>
#include <type_traits>

/*!
 * @brief 列挙型を基底型の整数値に変換する
 *
 * @param enum_val 変換する列挙型の値
 * @return 変換した基底型の整数値
 */
template <typename E>
    requires std::is_enum_v<E>
constexpr std::underlying_type_t<E> enum2i(E enum_val)
{
    return static_cast<std::underlying_type_t<E>>(enum_val);
}

/*!
 * @brief 整数値を列挙型の値に変換する
 *
 * @param integer_val 変換する整数値
 * @return 変換した列挙型の値
 */
template <typename E, std::integral I>
    requires std::is_enum_v<E>
constexpr E i2enum(I integer_val)
{
    return static_cast<E>(integer_val);
}

template <typename E, std::integral I>
    requires std::is_enum_v<E>
constexpr E operator+(E enum_val, I val)
{
    return i2enum<E>(enum2i(enum_val) + val);
}

template <typename E, std::integral I>
    requires std::is_enum_v<E>
constexpr E operator-(E enum_val, I val)
{
    return i2enum<E>(enum2i(enum_val) - val);
}

template <typename E>
    requires std::is_enum_v<E>
constexpr int operator-(E a, E b)
{
    return static_cast<int>(a) - static_cast<int>(b);
}
