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
#include "system/angband-system.h"
#include "system/h-basic.h"
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

/*!
 * @brief Random Number Generator -- Degree of "complex" RNG.
 *
 * This value is hard-coded at 63 for a wide variety of reasons.
 */
constexpr auto RAND_DEG = 63;

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
    return static_cast<T>(max > 0 ? rand_range(0, max - 1) : -rand_range(0, -max - 1));
}

template <typename T>
int randint0(T max)
    requires std::is_integral_v<T> || std::is_enum_v<T>
{
    return randnum0<int>(static_cast<int>(max));
}

/*!
 * @brief 平均値±振れ幅 の一様乱数を返す
 * @param average 平均値
 * @param width 振れ幅
 * @return 乱数値
 */
template <typename T>
int rand_spread(T average, T width)
{
    const auto abs_width = static_cast<int>(width);
    return static_cast<int>(average) + randint0(1 + 2 * std::abs(abs_width)) - std::abs(abs_width);
}

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

/*!
 * @brief 指定されたパーセンテージで事象が生起するかを返す
 * @param p 確率
 * @return 生起するか否か
 */
template <typename T>
bool evaluate_percent(T p)
{
    return randint0(100) < static_cast<int>(p);
}

/*!
 * @brief 1/nの確率で事象が生起するかを返す
 * @param n 母数
 * @return 生起するか否か
 */
template <typename T>
bool one_in_(T n)
{
    return randint0(static_cast<int>(n)) == 0;
}

void Rand_state_init();
int16_t randnor(int mean, int stand);
int32_t div_round(int32_t n, int32_t d);
int32_t Rand_external(int32_t m);

// clang-format off
template <typename T>
concept DistributionProducer = requires(T &dist) {
    { dist(std::declval<Xoshiro128StarStar &>()) } -> std::same_as<typename T::result_type>;
};
// clang-format on

/*!
 * @brief 引数で指定した分布生成器とゲームの乱数生成器から乱数を生成する
 *
 * @tparam T 分布生成器の型
 * @param dist 分布生成器
 * @return 生成した乱数の値を返す
 */
template <DistributionProducer T>
typename T::result_type rand_dist(T &dist)
{
    return dist(AngbandSystem::get_instance().get_rng());
}

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
