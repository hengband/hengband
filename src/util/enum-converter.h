#pragma once

#include <type_traits>

/*!
 * @brief 列挙型を基底型の整数値に変換する
 *
 * @tparam T 列挙型(enum もしくは enum class)
 * @param enum_val 変換する列挙型の値
 * @return 変換した基底型の整数値
 */
template <typename T>
constexpr std::underlying_type_t<T> enum2i(T enum_val)
{
    static_assert(std::is_enum_v<T>);

    return static_cast<std::underlying_type_t<T>>(enum_val);
}
