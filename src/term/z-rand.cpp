/*!
 * @file z-rand.cpp
 * @brief ゲームで使用する乱数ユーティリティの実装
 */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "term/z-rand.h"
#include "system/angband-system.h"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <tl/optional.hpp>

namespace {
/*!
 * @brief 0以上range未満の一様乱数を返す
 * @param rng 乱数生成器
 * @param range 値域の幅 (1以上)
 * @return 乱数値
 * @details Lemire の nearly-divisionless 法 (D. Lemire, "Fast Random Integer Generation in an Interval", 2019) による。
 * 乱数生成器の出力と range の64bit積の上位32bitを結果とし、下位32bitが閾値 2^32 mod range 未満の場合は
 * 引き直すことで偏りをなくしている。大半の場合は乗算1回で済み、剰余の計算も稀にしか行わない。
 */
uint32_t uniform_below(xso::rng32 &rng, uint32_t range)
{
    auto product = static_cast<uint64_t>(rng()) * range;
    auto low = static_cast<uint32_t>(product);
    if (low < range) {
        const auto threshold = (0u - range) % range;
        while (low < threshold) {
            product = static_cast<uint64_t>(rng()) * range;
            low = static_cast<uint32_t>(product);
        }
    }

    return static_cast<uint32_t>(product >> 32);
}

/*!
 * @brief a以上b以下の一様乱数を返す
 * @param rng 乱数生成器
 * @param a 最小値
 * @param b 最大値
 * @return 乱数値
 * @details a >= b の場合は乱数を消費せず a を返す。
 */
int uniform_int(xso::rng32 &rng, int a, int b)
{
    if (a >= b) {
        return a;
    }

    const auto width = static_cast<uint32_t>(static_cast<int64_t>(b) - a);
    const auto offset = (width == std::numeric_limits<uint32_t>::max()) ? rng() : uniform_below(rng, width + 1);
    return static_cast<int>(static_cast<int64_t>(a) + offset);
}

/*!
 * @brief 値を型 T の範囲に収める
 * @tparam T 変換先の整数型
 * @param value 値
 * @return T の範囲に収めた値
 */
template <std::integral T>
T clamp_to(int64_t value)
{
    return static_cast<T>(std::clamp<int64_t>(value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
}
}

/*!
 * @brief 乱数生成器の状態を初期化する
 * @param seed 初期シード。指定しない場合は実行毎に異なる乱数で初期化する
 * @details シードを指定すると同じ乱数列を再現できる (バグ再現・修正確認用)
 */
void Rand_state_init(tl::optional<uint32_t> seed)
{
    auto &rng = AngbandSystem::get_instance().get_rng();
    if (seed) {
        rng.seed(*seed);
        return;
    }

    rng.seed();
}

/*!
 * @brief a以上b以下の一様乱数を返す
 * @param a 最小値
 * @param b 最大値
 * @return 乱数値
 * @details a >= b の場合は乱数を消費せず a を返す。
 * rand_range(0, n - 1) は randint0(n) と同じ値を返す。
 */
int rand_range(int a, int b)
{
    return uniform_int(AngbandSystem::get_instance().get_rng(), a, b);
}

/*!
 * @brief 正規分布に従う整数の乱数を返す
 * @param mean 平均
 * @param stand 標準偏差
 * @return 乱数値 (int16_t の範囲に収めた値)
 * @details 16bitの一様乱数12個の和から平均を引いた値が、近似的に標準偏差 2^16 の正規分布に従うこと
 * (Irwin–Hall 分布) を利用して、整数演算のみで生成する。そのため結果は平均±6σの範囲で打ち切られる。
 * 端数は0から遠い方へ丸める。stand <= 0 の場合は乱数を消費せず mean を返す。
 */
int16_t randnor(int mean, int stand)
{
    if (stand <= 0) {
        return clamp_to<int16_t>(mean);
    }

    auto &rng = AngbandSystem::get_instance().get_rng();
    int64_t sum = 0;
    for (auto i = 0; i < 6; ++i) {
        const auto value = rng();
        sum += (value >> 16) + (value & 0xFFFF);
    }

    constexpr int64_t sum_mean = 6 * 0xFFFF; // 12 * 65535 / 2
    const auto offset = static_cast<int64_t>(stand) * (sum - sum_mean);
    const auto abs_scaled = (std::abs(offset) + 0x8000) >> 16;
    return clamp_to<int16_t>(static_cast<int64_t>(mean) + (offset < 0 ? -abs_scaled : abs_scaled));
}

/*!
 * @brief 除算の端数を乱数で丸めた商を返す
 * @param n 被除数
 * @param d 除数
 * @return 商
 * @details 端数 |n mod d| / |d| の確率で商の絶対値を1増やすことで、期待値が n / d に一致するようにする。
 * d == 0 の場合は n を返す。
 */
int32_t div_round(int32_t n, int32_t d)
{
    if (d == 0) {
        return n;
    }

    const auto n64 = static_cast<int64_t>(n);
    const auto d64 = static_cast<int64_t>(d);
    const auto abs_d = std::abs(d64);
    const auto remainder = std::abs(n64) % abs_d;
    auto quotient = n64 / d64;
    if (remainder > rand_range(0, static_cast<int>(abs_d - 1))) {
        quotient += ((n < 0) == (d < 0)) ? 1 : -1;
    }

    return clamp_to<int32_t>(quotient);
}

/*!
 * @brief ゲームの乱数生成器を使わずに0以上m未満の一様乱数を返す
 * @param m 値域の幅
 * @return 乱数値 (m <= 0 の場合は0)
 * @details BGMの選択などゲームの進行に影響しない場面で使う。
 * ゲームの乱数生成器の状態を変えないため、固定シードでのゲームの再現性に影響しない。
 */
int32_t Rand_external(int32_t m)
{
    if (m <= 0) {
        return 0;
    }

    static xso::rng32 urbg_external;
    return uniform_int(urbg_external, 0, m - 1);
}
