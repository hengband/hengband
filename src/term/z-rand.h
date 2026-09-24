/*!
 * @file z-rand.h
 * @brief ゲームで使用する乱数ユーティリティ
 * @details
 * 乱数生成器はゲーム全体で共有する AngbandSystem::get_rng() (xso::rng32) を使う。
 * 一様分布・正規分布などの分布の変換はすべて自前のアルゴリズムで実装しており、
 * 同じシードからはプラットフォームや標準ライブラリの実装によらず同じ乱数列が得られる。
 *
 * @note xso::rng32 のメンバ関数 (sample(), shuffle(), flip() など) や標準ライブラリの分布
 * (std::uniform_int_distribution など) は、実装依存のアルゴリズムで値を生成するため、
 * ゲームの乱数生成器に対して使ってはならない。
 */

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
#include "util/type-concepts.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <tl/optional.hpp>

/*!
 * @brief セーブファイルに記録する乱数生成器の状態の語数
 * @details 旧来の乱数生成器 (BSD-degree-63-RNG) の名残。
 * 現在の乱数生成器 (xso::rng32) の状態はこれより少ない語数で表されるが、
 * セーブファイルの互換性を保つため、状態の記録領域はこの語数分確保し、余りは0で埋める。
 */
constexpr auto RAND_DEG = 63;

void Rand_state_init(tl::optional<uint32_t> seed = tl::nullopt);
int rand_range(int a, int b);
int16_t randnor(int mean, int stand);
int32_t div_round(int32_t n, int32_t d);
int32_t Rand_external(int32_t m);

/*!
 * @brief 0以上/0以下の一様乱数を返す
 * @tparam T 戻り値の型
 * @tparam U 引数の型
 * @param initial_max 最大値 (負ならば最小値)
 * @return 乱数値
 * @details initial_max > 0 ならば 0 以上 initial_max 未満、initial_max < 0 ならば initial_max を超え 0 以下、
 * initial_max == 0 ならば 0 を返す。
 */
template <IntegralOrEnum T, IntegralOrEnum U>
T randnum0(U initial_max)
{
    const auto max = static_cast<int>(initial_max);
    return static_cast<T>(max > 0 ? rand_range(0, max - 1) : -rand_range(0, -max - 1));
}

/*!
 * @brief 0以上/0以下の一様乱数を int で返す
 * @tparam T 引数の型
 * @param max 最大値 (負ならば最小値)
 * @return 乱数値
 * @details 値域は randnum0() と同じ。
 */
template <IntegralOrEnum T>
int randint0(T max)
{
    return randnum0<int>(max);
}

/*!
 * @brief 1以上/-1以下の一様乱数を返す
 * @tparam T 戻り値の型
 * @tparam U 引数の型
 * @param initial_max 最大値 (負ならば最小値)
 * @return 乱数値
 * @details initial_max >= 1 ならば 1 以上 initial_max 以下、initial_max <= -1 ならば initial_max 以上 -1 以下、
 * initial_max == 0 ならば 1 を返す。
 */
template <IntegralOrEnum T, IntegralOrEnum U>
T randnum1(U initial_max)
{
    const auto max = static_cast<int>(initial_max);
    if (max == 0) {
        return static_cast<T>(1);
    }

    return static_cast<T>(max > 0 ? rand_range(1, max) : -rand_range(1, -max));
}

/*!
 * @brief 1以上/-1以下の一様乱数を int で返す
 * @tparam T 引数の型
 * @param max 最大値 (負ならば最小値)
 * @return 乱数値
 * @details 値域は randnum1() と同じ。
 */
template <IntegralOrEnum T>
int randint1(T max)
{
    return randnum1<int>(max);
}

/*!
 * @brief 平均値±振れ幅 の一様乱数を返す
 * @tparam T 引数の型
 * @param average 平均値
 * @param width 振れ幅 (負の場合は絶対値を使う)
 * @return average - |width| 以上 average + |width| 以下の乱数値
 */
template <IntegralOrEnum T>
int rand_spread(T average, T width)
{
    const auto abs_width = std::abs(static_cast<int>(width));
    return static_cast<int>(average) + randint0(1 + 2 * abs_width) - abs_width;
}

/*!
 * @brief 指定されたパーセンテージで事象が生起するかを返す
 * @tparam T 引数の型
 * @param p 確率 (%)
 * @return 生起するか否か
 * @details p <= 0 ならば常に false、p >= 100 ならば常に true を返す。
 */
template <IntegralOrEnum T>
bool evaluate_percent(T p)
{
    return randint0(100) < static_cast<int>(p);
}

/*!
 * @brief 1/nの確率で事象が生起するかを返す
 * @tparam T 引数の型
 * @param n 母数
 * @return 生起するか否か
 * @details n が 0 もしくは ±1 ならば常に true を返す。
 */
template <IntegralOrEnum T>
bool one_in_(T n)
{
    return randint0(n) == 0;
}

/*!
 * @brief イテレータの範囲 [first,last) の要素を等確率で並び替える (Fisher-Yates shuffle)
 * @tparam Iter イテレータの型
 * @param first 範囲の先頭を指すイテレータ
 * @param last 範囲の終端を指すイテレータ
 */
template <std::random_access_iterator Iter>
    requires std::permutable<Iter>
void rand_shuffle(Iter first, Iter last)
{
    for (auto n = last - first - 1; n > 0; --n) {
        const auto m = randint0(n + 1);
        std::iter_swap(first + n, first + m);
    }
}

/*!
 * @brief 与えられた範囲から等確率で要素を1つ選ぶ
 * @tparam R 範囲の型
 * @param range 要素を選ぶ範囲
 * @return 選んだ要素 (範囲の要素の参照型)
 * @details 範囲が一時オブジェクトの場合、戻り値がダングリング参照にならないよう
 * 範囲が std::ranges::borrowed_range であることを要求する。
 * @exception std::out_of_range 範囲が空の場合
 */
template <std::ranges::forward_range R>
    requires std::ranges::sized_range<R> && std::ranges::borrowed_range<R>
decltype(auto) rand_choice(R &&range)
{
    const auto size = std::ranges::size(range);
    if (size == 0) {
        THROW_EXCEPTION(std::out_of_range, "Cannot choose an element from an empty range.");
    }

    const auto index = randint0(size);
    return *std::ranges::next(std::ranges::begin(range), index);
}

/*!
 * @brief 与えられたリストから等確率で要素を1つ選ぶ
 * @tparam T リストの要素の型
 * @param list 要素を選ぶリスト
 * @return 選んだ要素のコピー
 * @exception std::out_of_range リストが空の場合
 */
template <typename T>
T rand_choice(std::initializer_list<T> list)
{
    if (list.size() == 0) {
        THROW_EXCEPTION(std::out_of_range, "Cannot choose an element from an empty list.");
    }

    const auto index = randint0(list.size());
    return *(list.begin() + index);
}
