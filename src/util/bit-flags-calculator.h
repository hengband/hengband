#pragma once

#include <cassert>
#include <type_traits>

#define has_flag(ARRAY, INDEX) !!((ARRAY)[(INDEX) / 32] & (1UL << ((INDEX) % 32)))
#define add_flag(ARRAY, INDEX) ((ARRAY)[(INDEX) / 32] |= (1UL << ((INDEX) % 32)))
#define remove_flag(ARRAY, INDEX) ((ARRAY)[(INDEX) / 32] &= ~(1UL << ((INDEX) % 32)))
#define is_pval_flag(INDEX) ((TR_STR <= (INDEX) && (INDEX) <= TR_MAGIC_MASTERY) || (TR_STEALTH <= (INDEX) && (INDEX) <= TR_BLOWS))
#define has_pval_flags(ARRAY) !!((ARRAY)[0] & (0x00003f7f))

template <typename T, typename U>
void bit_flags_calculator_assert(T, [[maybe_unused]] U bits)
{
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
    if constexpr (std::is_integral_v<T>) {
        static_assert(std::is_unsigned_v<T>);
    }
    if constexpr (sizeof(T) < sizeof(U)) {
        assert(static_cast<T>(bits) == bits);
    }
}

/*!
 * @brief フラグ変数の指定したビットをセットする
 *
 * @param flag 操作の対象となるフラグ変数
 * @param bits セットする対象のビットがONであるビットパターン
 */
template <typename T, typename U>
void set_bits(T &flag, U bits)
{
    bit_flags_calculator_assert(flag, bits);
    flag |= static_cast<T>(bits);
}

/*!
 * @brief フラグ変数の指定したビットをリセットする
 *
 * @param flag 操作の対象となるフラグ変数
 * @param bits リセットする対象のビットがONであるビットパターン
 */
template <typename T, typename U>
void reset_bits(T &flag, U bits)
{
    bit_flags_calculator_assert(flag, bits);
    flag &= ~static_cast<T>(bits);
}

/*!
 * @brief フラグ変数の指定したビットがすべてONかどうか検査する
 *
 * @param flag 検査する対象となるフラグ変数
 * @param bits 検査する対象のビットがONであるビットパターン
 * @return bitsで指定したビットがすべてONであればtrue、そうでなければfalse
 */
template <typename T, typename U>
bool all_bits(T flag, U bits)
{
    bit_flags_calculator_assert(flag, bits);
    return (flag & static_cast<T>(bits)) == static_cast<T>(bits);
}

/*!
 * @brief フラグ変数の指定したビットのいずれかがONかどうか検査する
 *
 * @param flag 検査する対象となるフラグ変数
 * @param bits 検査する対象のビットがONであるビットパターン
 * @return bitsで指定したビットのいずれかがONであればtrue、そうでなければfalse
 */
template <typename T, typename U>
bool any_bits(T flag, U bits)
{
    bit_flags_calculator_assert(flag, bits);
    return (flag & static_cast<T>(bits)) != 0;
}

/*!
 * @brief フラグ変数の指定したビットがすべてOFFかどうか検査する
 *
 * @param flag 検査する対象となるフラグ変数
 * @param bits 検査する対象のビットがONであるビットパターン
 * @return bitsで指定したビットがすべてOFFであればtrue、そうでなければfalse
 */
template <typename T, typename U>
bool none_bits(T flag, U bits)
{
    return !any_bits(flag, bits);
}

/*!
 * @brief フラグ変数の指定したビットにおいて、指定したビットパターンとON/OFFがすべて一致するかどうかを検査する
 *
 * @param flag 検査する対象となるフラグ変数
 * @param bits 検査する対象のビットがONであるビットパターン
 * @param match bitsで指定したビットにおいて、ON/OFFがすべて一致するかどうか比較するビットパターン
 * @return すべて一致すればtrue、そうでなければfalse
 */
template <typename T, typename U>
bool match_bits(T flag, U bits, U match)
{
    bit_flags_calculator_assert(flag, bits);
    bit_flags_calculator_assert(flag, match);
    return (flag & static_cast<T>(bits)) == static_cast<T>(match);
}
