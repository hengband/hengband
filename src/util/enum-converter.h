#pragma once

#include <type_traits>

/*!
 * @brief 列挙型を基底型の整数値に変換する
 *
 * @tparam EnumType 列挙型(enum もしくは enum class)
 * @param enum_val 変換する列挙型の値
 * @return 変換した基底型の整数値
 */
template <typename EnumType>
constexpr std::underlying_type_t<EnumType> enum2i(EnumType enum_val)
{
    static_assert(std::is_enum_v<EnumType>);

    return static_cast<std::underlying_type_t<EnumType>>(enum_val);
}

/*!
 * @brief 整数値を列挙型の値に変換する
 *
 * @tparam EnumType 変換先の列挙型
 * @tparam IntegerType 変換元の整数値の型
 * @param integer_val 変換する整数値
 * @return 変換した列挙型の値
 */
template <typename EnumType, typename IntegerType>
constexpr EnumType i2enum(IntegerType integer_val)
{
    static_assert(std::is_integral_v<IntegerType>);
    static_assert(std::is_enum_v<EnumType>);

    return static_cast<EnumType>(integer_val);
}

template <typename EnumType, std::enable_if_t<std::is_enum_v<EnumType>, std::nullptr_t> = nullptr>
constexpr EnumType operator+(EnumType enum_val, int val)
{
    return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(enum_val) + val);
}

template <typename EnumType, std::enable_if_t<std::is_enum_v<EnumType>, std::nullptr_t> = nullptr>
constexpr EnumType operator-(EnumType enum_val, int val)
{
    return operator+(enum_val, -val);
}

template <typename EnumType, std::enable_if_t<std::is_enum_v<EnumType>, std::nullptr_t> = nullptr>
constexpr int operator-(EnumType a, EnumType b)
{
    return static_cast<int>(a) - static_cast<int>(b);
}
