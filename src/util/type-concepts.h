#pragma once

#include <concepts>
#include <type_traits>

/*!
 * @brief 整数型もしくは列挙型であることを表すコンセプト
 */
template <typename T>
concept IntegralOrEnum = std::integral<T> || std::is_enum_v<T>;
