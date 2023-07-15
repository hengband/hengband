#include "util/rng-xoshiro.h"

namespace {

/*!
 * @brief 32ビットデータを右ローテートする
 *
 * @param x 右ローテートする32ビットデータ
 * @param k 右ローテートするビット数
 * @return xを右にkビットローテートした32ビットデータを返す
 */
uint32_t u32b_rotl(uint32_t x, int k)
{
    return (x << k) | (x >> (32 - k));
}

}

/*!
 * @brief デフォルトシードで乱数の内部状態を初期化したXoshiro128StarStarクラスのオブジェクトを生成する
 */
Xoshiro128StarStar::Xoshiro128StarStar()
    : rng_state{ {
          // default seeds
          123456789,
          362436069,
          521288629,
          88675123,
      } }
{
}

/*!
 * @brief 引数で与えたシードを元に乱数の内部状態を初期化したXoshiro128StarStarクラスのオブジェクトを生成する
 *
 * @param seed 乱数の内部状態を初期化する元となるシード
 */
Xoshiro128StarStar::Xoshiro128StarStar(uint32_t seed)
{
    this->set_state(seed);
}

/*!
 * @brief 次の乱数を生成し、内部状態を更新する
 *
 * @return 生成した乱数を返す
 */
Xoshiro128StarStar::result_type Xoshiro128StarStar::operator()()
{
    auto &s = this->rng_state;

    const uint32_t result = u32b_rotl(s[1] * 5, 7) * 9;

    const uint32_t t = s[1] << 9;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = u32b_rotl(s[3], 11);

    return result;
}

/*!
 * @brief 乱数の内部状態をセットする
 *
 * @param state 乱数の内部状態
 */
void Xoshiro128StarStar::set_state(const state_type &state)
{
    this->rng_state = state;
}

/*!
 * @brief シードを元に乱数の内部状態をセットする
 *
 * @param seed 乱数の内部状態の元とするシード
 */
void Xoshiro128StarStar::set_state(uint32_t seed)
{
    auto i = 1;
    for (auto &s : this->rng_state) {
        seed = 1812433253UL * (seed ^ (seed >> 30)) + i;
        s = seed;
        ++i;
    }
}

/**
 * @brief 乱数の内部状態を取得する
 *
 * @return 乱数の内部状態への参照
 */
const Xoshiro128StarStar::state_type &Xoshiro128StarStar::get_state() const
{
    return this->rng_state;
}
