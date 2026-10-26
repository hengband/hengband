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
#include <array>
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
 * @brief 64bit値をかき混ぜる
 * @param value 値
 * @return かき混ぜた値
 * @details SplitMix64 (G. Steele, D. Lea, C. Flood, "Fast Splittable Pseudorandom Number Generators", 2014) の
 * 1 ステップ分 (定数の加算と出力関数) と同じ。
 */
constexpr uint64_t mix64(uint64_t value)
{
    value += 0x9E3779B97F4A7C15;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EB;
    return value ^ (value >> 31);
}

/*!
 * @brief make_keyed_rng() で混ぜ込むシードを返す
 * @return シードへの参照
 * @details Rand_external_state_init() で初期シードを指定しない限り、実行毎に異なる値になる。
 */
uint32_t &get_keyed_rng_seed()
{
    static uint32_t seed = xso::rng32()();
    return seed;
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
    auto &rng = get_game_rng();
    if (seed) {
        rng.seed(*seed);
        return;
    }

    rng.seed();
}

/*!
 * @brief ゲームの進行に影響しない乱数生成器の状態を初期化する
 * @param seed 初期シード。指定しない場合は実行毎に異なる乱数で初期化する
 * @details ゲームの乱数生成器と同じ初期シードを指定しても同じ乱数列にならないよう、シードを変えて初期化する。
 * シードを指定すると、同じキー入力に対して幻覚の表示などゲームの進行に影響しない乱数も再現できる。
 * make_keyed_rng() が作る乱数生成器のシードもここで初期化する。
 */
void Rand_external_state_init(tl::optional<uint32_t> seed)
{
    auto &rng = get_external_rng();
    if (seed) {
        constexpr uint32_t salt = 0x45585452; // "EXTR"
        rng.seed(*seed ^ salt);
        constexpr uint32_t keyed_salt = 0x4B455944; // "KEYD"
        get_keyed_rng_seed() = *seed ^ keyed_salt;
        return;
    }

    rng.seed();
    get_keyed_rng_seed() = xso::rng32()();
}

/*!
 * @brief ゲームの乱数生成器を返す
 * @return ゲームの乱数生成器 (AngbandSystem::get_rng())
 * @details 乱数生成器を引数に取らない乱数関数はこれを使う。
 */
xso::rng32 &get_game_rng()
{
    return AngbandSystem::get_instance().get_rng();
}

/*!
 * @brief ゲームの進行に影響しない乱数生成器を返す
 * @return ゲームの乱数生成器とは独立した乱数生成器
 * @details 描画など呼び出される回数が環境によって変わる処理や、ゲームの時間が経過しない操作で使う。
 * Rand_external_state_init() で初期シードを指定しない限り実行毎に異なるシードで初期化され、
 * セーブファイルにも記録されない。
 */
xso::rng32 &get_external_rng()
{
    static xso::rng32 rng_external;
    return rng_external;
}

/*!
 * @brief キーから決まる乱数生成器を作る
 * @param purpose 用途
 * @param keys キー (ゲームのターンや座標など)
 * @return 乱数生成器
 * @details シード (Rand_external_state_init() で初期化する)・用途・キーが同じなら、常に同じ乱数列を返す。
 * 描画など呼び出される回数が環境によって変わる処理で使うと、表示がゲームの状態だけで決まり、
 * 描き直しの回数によらず固定シードで再現できる。ゲームの乱数生成器も get_external_rng() も消費しない。
 */
xso::rng32 make_keyed_rng(KeyedRngPurpose purpose, std::initializer_list<int> keys)
{
    auto hash = mix64(get_keyed_rng_seed());
    hash = mix64(hash ^ static_cast<uint32_t>(purpose));
    for (const auto key : keys) {
        hash = mix64(hash ^ static_cast<uint32_t>(key));
    }

    const auto hash2 = mix64(hash);
    const std::array<uint32_t, xso::rng32::word_count()> state{ {
        static_cast<uint32_t>(hash),
        static_cast<uint32_t>(hash >> 32),
        static_cast<uint32_t>(hash2),
        static_cast<uint32_t>(hash2 >> 32),
    } };
    return xso::rng32(state.begin(), state.end());
}

/*!
 * @brief 指定した乱数生成器で a以上b以下の一様乱数を返す
 * @param rng 乱数生成器
 * @param a 最小値
 * @param b 最大値
 * @return 乱数値
 * @details a >= b の場合は乱数を消費せず a を返す。
 */
int rand_range(xso::rng32 &rng, int a, int b)
{
    if (a >= b) {
        return a;
    }

    const auto width = static_cast<uint32_t>(static_cast<int64_t>(b) - a);
    const auto offset = (width == std::numeric_limits<uint32_t>::max()) ? rng() : uniform_below(rng, width + 1);
    return static_cast<int>(static_cast<int64_t>(a) + offset);
}

/*!
 * @brief a以上b以下の一様乱数を返す
 * @param a 最小値
 * @param b 最大値
 * @return 乱数値
 * @details ゲームの乱数生成器を使う以外は rand_range(xso::rng32 &, int, int) と同じ。
 * rand_range(0, n - 1) は randint0(n) と同じ値を返す。
 */
int rand_range(int a, int b)
{
    return rand_range(get_game_rng(), a, b);
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

    auto &rng = get_game_rng();
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
 * @details get_external_rng() を使う。日記の副題の選択などゲームの進行に影響しない場面で使う。
 * ゲームの乱数生成器の状態を変えないため、固定シードでのゲームの再現性に影響しない。
 */
int32_t Rand_external(int32_t m)
{
    return rand_range(get_external_rng(), 0, m - 1);
}
