/* File: z-rand.h */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#pragma once

#include "system/angband-exceptions.h"
#include "system/h-basic.h"
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

/**** Available constants ****/

/*
 * Random Number Generator -- Degree of "complex" RNG -- see "misc.c"
 * This value is hard-coded at 63 for a wide variety of reasons.
 */
#define RAND_DEG 63

/*
 * Generates a random long integer X where A<=X<=B
 * The integer X falls along a uniform distribution.
 * Note: rand_range(0,N-1) == randint0(N)
 */
int rand_range(int a, int b);

/*!
 * @brief 0以上/以下の一様乱数を返す
 * @param 最大値 (負ならば最小値)
 * @return 乱数出力
 * @details max > 0 ならば0以上max未満、max < 0 ならばmaxを超え0以下、0ならば0
 */
template <typename T, typename U>
T randnum0(U initial_max)
    requires(std::is_integral_v<T> || std::is_enum_v<T>) && (std::is_integral_v<U> || std::is_enum_v<U>)
{
    const auto max = static_cast<int>(initial_max);
    return max > 0 ? static_cast<T>(rand_range(0, max - 1)) : -static_cast<T>(rand_range(0, -max - 1));
}

template <typename T>
int randint0(T max)
    requires std::is_integral_v<T> || std::is_enum_v<T>
{
    return randnum0<int>(static_cast<int>(max));
}

/*
 * Generate a random long integer X where A-D<=X<=A+D
 * The integer X falls along a uniform distribution.
 * Note: rand_spread(A,D) == rand_range(A-D,A+D)
 */
#define rand_spread(A, D) ((A) + (randint0(1 + (D) + (D))) - (D))

/*!
 * @brief 1以上/-1以下の一様乱数を返す
 * @return 最大値 (負ならば最小値)
 * @return 乱数出力
 * @details max > 1 ならば1以上max以下、max < -1 ならばmax以上-1以下、-1は入力値、0～+1は1
 */
template <typename T, typename U>
T randnum1(U initial_max)
    requires(std::is_integral_v<T> || std::is_enum_v<T>) && (std::is_integral_v<U> || std::is_enum_v<U>)
{
    const auto max = static_cast<int>(initial_max);
    if (max == 0) {
        return static_cast<T>(1);
    }

    return max > 0 ? static_cast<T>(rand_range(1, max)) : static_cast<T>(-rand_range(1, -max));
}

template <typename T>
int randint1(T max)
    requires std::is_integral_v<T> || std::is_enum_v<T>
{
    return randnum1<int>(static_cast<int>(max));
}

/*
 * Evaluate to TRUE "P" percent of the time
 */
#define magik(P) (randint0(100) < (P))

/*
 * Evaluate to TRUE with probability 1/x
 */
#define one_in_(X) (randint0(X) == 0)

/*
 * Evaluate to TRUE "S" percent of the time
 */
#define saving_throw(S) (randint0(100) < (S))

void Rand_state_init(void);
int16_t randnor(int mean, int stand);
int16_t damroll(DICE_NUMBER num, DICE_SID sides);
int16_t maxroll(DICE_NUMBER num, DICE_SID sides);
int32_t div_round(int32_t n, int32_t d);
int32_t Rand_external(int32_t m);

template <typename>
struct is_reference_wrapper : std::false_type {
};
template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {
};

/*!
 * @brief 型 T が std::reference_wrappter かどうか調べる
 */
template <typename T>
constexpr auto is_reference_wrapper_v = is_reference_wrapper<T>::value;

/*!
 * @brief イテレータの範囲 [first,last) のそれぞれの要素を同じ確率で並び替える
 * ※ イテレータの指す要素が std::reference_wrapper<T> の場合は要素の値ではなく、要素が保持している参照先の値を入れ替える。
 * @tparam Iter イテレータの型
 * @param first 範囲の先頭を指すイテレータ
 * @param last 範囲の終端を指すイテレータ
 */
template <typename Iter>
void rand_shuffle(Iter first, Iter last)
{
    using value_type = typename std::iterator_traits<Iter>::value_type;

    for (auto n = std::distance(first, last) - 1; n > 0; --n) {
        using std::swap;
        const auto m = randint0(n + 1);

        if constexpr (is_reference_wrapper_v<value_type>) {
            swap(first[n].get(), first[m].get());
        } else {
            swap(first[n], first[m]);
        }
    }
}

/*!
 * @brief 与えられた範囲から等確率で要素を1つ選ぶ
 *
 * @tparam T 範囲の型
 * @param range 要素を選ぶ範囲
 * @return 選んだ要素
 *
 * @todo MacOSでApple ClangのC++20 Ranges Libraryのサポートが浸透したら
 * requires std::ranges::borrowed_range<T> && std::ranges::sized_range<T>
 * をコンセプトに指定する
 */
template <typename T>
decltype(auto) rand_choice(T &&range)
{
    using std::begin;
    using std::size;
    const auto index = randint0(size(range));

    return *(std::next(begin(range), index));
}

/*!
 * @brief 与えられたリストから等確率で要素を1つ選ぶ
 *
 * @tparam T リストの要素の型
 * @param list 要素を選ぶリスト
 * @return 選んだ要素
 */
template <typename T>
T rand_choice(std::initializer_list<T> list)
{
    const auto index = randint0(list.size());

    return *(list.begin() + index);
}
