#pragma once

#include <array>
#include <cstdint>

#ifdef WINDOWS
// windows.h をインクルードすると min と max マクロが勝手に定義されるという迷惑な仕様があるため、
// undef しておかないと Xoshiro128StarStar::min/max の宣言がエラーになる
#undef min
#undef max
#endif

/*!
 * @brief 乱数生成器 xoshiro128** クラス
 * @details 乱数生成器 xoshiro128** (https://prng.di.unimi.it/) を STL の <random> ヘッダで提供される
 * 各種アルゴリズムが使用できる Uniform Random Bit Generator として実装したクラス
 */
class Xoshiro128StarStar {
public:
    using result_type = uint32_t;
    using state_type = std::array<uint32_t, 4>;

    Xoshiro128StarStar();
    explicit Xoshiro128StarStar(uint32_t seed);

    static constexpr result_type min()
    {
        return 0;
    }
    static constexpr result_type max()
    {
        return ~min();
    }

    result_type operator()();

    void set_state(uint32_t seed);

    void set_state(const state_type &state);
    const state_type &get_state() const;

private:
    state_type rng_state; //!< RNG state
};
