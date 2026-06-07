/// @brief An implementation of the xoshiro/xoroshiro family of pseudorandom number generators.
/// @link  https://nessan.github.io/xoshiro has the complete documentation.
///
/// SPDX-FileCopyrightText:  2023 Nessan Fitzmaurice <nessan.fitzmaurice@me.com>
/// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <concepts>
#include <format>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>

namespace xso {

/// @brief A C++ concept that lets us distinguish e.g. a @c std::normal_distribution from a @c std::vector
/// @note  Relies on STL distributions defining a @c param_type type that STL containers do not.
template<typename D>
concept Distribution = requires { typename D::param_type; };

/// @brief  The main `xso::generator` class combines a State and a Scrambler to create a PRNG.
/// @tparam State     Provides access to the words of state, and has a @c step() method to advance them.
/// @tparam Scrambler Functor to reduce the State to a single output word (a 32 or 64 bit unsigned).
/// @note   Most users will not use this directly but instead reference one of the type aliased generators below.
template<typename State, typename Scrambler>
class generator {
public:
    /// @brief The State type for the generator.
    using state_type = State;

    /// @brief The Scrambler type for the generator.
    using scrambler_type = Scrambler;

    /// @brief The state is packed into words of this type (in practice, 32 or 64 bit unsigneds).
    using word_type = typename State::word_type;

    /// @brief Returns the number of words of state.
    static constexpr std::size_t word_count() { return State::word_count(); }

    /// @brief Returns the number of bits of state.
    static constexpr std::size_t bit_count() { return State::bit_count(); }

    /// @brief Each call to generator() returns a single unsigned integer of this type.
    /// @note  Required by the @c UniformRandomBitGenerator concept.
    /// @note  For our generators this is always the same type as @c word_type.
    using result_type = word_type;

    /// @brief Returns the smallest value this generator can produce.
    /// @note  Required by the @c UniformRandomBitGenerator concept.
    static constexpr result_type min() noexcept { return 0; }

    /// @brief Returns the largest value this generator can produce.
    /// @note  Required by the @c UniformRandomBitGenerator concept.
    static constexpr result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

    /// Returns a name for this generator.
    static constexpr auto xso_name() { return std::format("{}{}", State::xso_name(), Scrambler::xso_name()); }

    /// @brief Default constructor seeds the full state randomly.
    /// @note  This will produce a high quality stream of random outputs that are different on each run.
    generator() { seed(); }

    /// @brief Construct a generator quickly but @b not well from a single unsigned integer value.
    /// @note  Seeding from a single value is an easy way to get repeatable random streams.
    explicit generator(word_type s) { seed(s); }

    /// @brief Construct and seed from an iteration of words which are all copied into the state.
    /// @note  The words shouldn't all be zeros.
    template<typename Iter>
    explicit generator(Iter b, Iter e)
    {
        seed(b, e);
    }

    /// @brief Sets the full state to random starting values.
    /// @note  This will produce a high quality stream of random outputs that are different on each run.
    void seed()
    {
        // We will use std::random_device as the principal source of entropy.
        std::random_device dev;

        // Fill a full seed array with calls to dev() --  may need a couple of dev() calls to fill one word of state.
        std::array<word_type, word_count()> full_state;
        if constexpr (sizeof(word_type) <= sizeof(std::random_device::result_type)) {
            for (auto& word : full_state) word = static_cast<word_type>(dev());
        }
        else {
            for (auto& word : full_state) word = static_cast<word_type>(static_cast<uint64_t>(dev()) << 32 | dev());
        }

        // std::random_device may be poor so we dd data from a call to a high resolution clock for first word.
        using clock_type = std::chrono::high_resolution_clock;
        auto ticks = static_cast<std::uint64_t>(clock_type::now().time_since_epoch().count());

        // However, from call to call, ticks only changes in the low order bits -- better scramble things a bit!
        ticks = murmur_scramble64(ticks);

        // Fold the scrambled ticks variable into the first seed word & seed the state.
        full_state[0] ^= static_cast<word_type>(ticks);
        m_state.seed(full_state.cbegin(), full_state.cend());
    }

    /// @brief Fill the state quickly but probably @b not well from a single unsigned integer value.
    /// @note  Seeding from a single value is an easy way to get repeatable random streams.
    constexpr void seed(word_type seed)
    {
        // Scramble the bits in the single seed we were given.
        auto sm64_state = murmur_scramble64(seed);

        // Use SplitMix64 to at least put some values in all the state words & seed the state.
        std::array<word_type, word_count()> full_state;
        for (auto& word : full_state) word = static_cast<word_type>(split_mix64(sm64_state));
        m_state.seed(full_state.cbegin(), full_state.cend());
    }

    /// @brief Set the state from an iteration of words which are all copied into the state.
    /// @note  The words shouldn't all be zeros.
    template<typename Iter>
    constexpr void seed(Iter b, Iter e)
    {
        m_state.seed(b, e);
    }

    /// @brief Advance the state by one step.
    constexpr void step() { m_state.step(); }

    /// @brief Reduce the current state to get a single @c result_type and then prep for the next call.
    /// @note  This method is required by the @c UniformRandomBitGenerator concept.
    constexpr result_type operator()()
    {
        result_type retval = m_scrambler(m_state);
        step();
        return retval;
    }

    /// @brief Read-only access to the i'th state word.
    constexpr word_type operator[](std::size_t i) const { return m_state[i]; }

    /// @brief Read-only access to the whole state which we copy into @c dst
    template<typename Iter>
    constexpr void get_state(Iter dst) const
    {
        m_state.get_state(dst);
    }

    /// @brief Returns a single integer value from a uniform distribution over @c [a,b].
    /// @note  No error checking is done and the behaviour is undefined if a > b.
    template<std::integral T>
    constexpr T sample(T a, T b)
    {
        return std::uniform_int_distribution<T>{a, b}(*this);
    }

    /// @brief Returns a single real value from a uniform distribution over @c [a,b).
    /// @note  No error checking is done and the behaviour is undefined if a > b.
    template<std::floating_point T>
    constexpr T sample(T a, T b)
    {
        return std::uniform_real_distribution<T>{a, b}(*this);
    }

    /// @brief Returns a single index from a uniform distribution over @c [0,len).
    /// @note  No error checking is done and the behaviour is undefined if len = 0.
    template<std::integral T>
    constexpr T index(T len)
    {
        return sample(T{0}, len - 1);
    }

    /// @brief Returns a single value from an iteration -- all elements are equally likely to be returned.
    /// @note  No error checking is done and the behaviour is undefined if e < b.
    template<std::input_iterator T>
    constexpr auto sample(T b, T e)
    {
        // Edge case?
        auto len = std::distance(b, e);
        if (len < 2) return *b;

        // Pick an index inside the iteration at random & return the corresponding value.
        auto i = index(len);
        std::advance(b, i);
        return *b;
    }

    /// @brief Returns a single value from a container -- all elements are equally likely to be returned.
    template<typename Container>
    constexpr auto sample(const Container& container)
    {
        return sample(std::cbegin(container), std::cend(container));
    }

    /// @brief Pick @c n elements from an iteration @c [b,e) without replacement & put the chosen samples in @c dst.
    /// @note  See the documentation for @c std::sample(...) for more details.
    /// @note  No error checking is done and the behaviour is undefined if e < b.
    template<std::input_iterator Src, typename Dst>
    constexpr Dst sample(Src b, Src e, Dst dst, std::unsigned_integral auto n)
    {
        return std::sample(b, e, dst, n, *this);
    }

    /// @brief Pick @c n elements from a container without replacement & put the chosen samples in @c dst.
    /// @note  See the documentation for @c std::sample(...) for more details.
    template<typename Src, typename Dst>
    constexpr auto sample(const Src& src, Dst dst, std::unsigned_integral auto n)
    {
        return sample(std::cbegin(src), std::cend(src), dst, n);
    }

    /// @brief Returns a single random variate drawn from a distribution.
    /// @param dist The distribution in question e.g. a @c std::normal_distribution object.
    constexpr auto sample(Distribution auto& dist) { return dist(*this); }

    /// @brief Pushes @c n samples from a distribution into a destination iterator.
    /// @param dist The distribution in question e.g. a @c std::normal_distribution object.
    /// @param dst  An iterator to the start of where we put the samples.
    template<typename Iter>
    constexpr Iter sample(Distribution auto& dist, Iter dst, std::unsigned_integral auto n)
    {
        while (n-- != 0) *dst++ = dist(*this);
        return dst;
    }

    /// @brief Roll a dice with an arbitrary arithmetic_type of sides (defaults to the usual 6).
    constexpr int roll(int n_sides = 6) { return sample(1, n_sides); }

    /// @brief Flip a coin,
    /// @param p Probability of getting a true uniform return -- defaults to a fair 50%.
    constexpr bool flip(double p = 0.5) { return std::bernoulli_distribution{p}(*this); }

    /// @brief Shuffles the elements in an iteration.
    /// @note  No error checking is done and the behaviour is undefined if e < b.
    template<std::random_access_iterator Iter>
    constexpr void shuffle(Iter b, Iter e)
    {
        std::shuffle(b, e, *this);
    }

    /// @brief Shuffles the elements of a container
    template<typename Container>
    constexpr void shuffle(Container& container)
    {
        return shuffle(std::begin(container), std::end(container));
    }

    /// @brief Discard the next @c z iterations in the random number sequence.
    /// @note  We can do much better for large @c z by using one of the @c jump methods.
    void discard(std::uint64_t z)
    {
        for (std::uint64_t i = 0; i < z; ++i) step();
    }

    /// @brief  If the state's characteristic polynomial is c(x) = x^n + p(x) where degree[p] < n then we
    ///         fill @c dst with the @b precomputed coefficients for p(x) = p_0 + p_1 x + ... + p_{n-1}.
    /// @throws If the @c State has no precomputed characteristic coefficients this fails.
    template<typename Iter>
    static constexpr void characteristic_coefficients(Iter dst)
    {
        return State::characteristic_coefficients(dst);
    }

private:
    State     m_state;
    Scrambler m_scrambler;

    /// @brief  The SplitMix64 random number generator -- a simple generator with 64 bits of state.
    /// @param  state The current value of the 64-bit state which is altered by this function.
    /// @return A 64-bit unsigned random output.
    static constexpr std::uint64_t split_mix64(std::uint64_t& state)
    {
        std::uint64_t z = (state += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        z = (z ^ (z >> 31));
        return z;
    };

    /// @brief Uses the murmur algorithm to return a word that is a scrambled version of the 64 input bits.
    static constexpr std::uint64_t murmur_scramble64(std::uint64_t x)
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53L;
        x ^= x >> 33;
        return x;
    }

    /// @brief Uses the murmur algorithm to return a word that is a scrambled version of the 32 input bits.
    static constexpr std::uint32_t murmur_scramble32(std::uint32_t x)
    {
        x *= 0xcc9e2d51;
        x = (x << 15) | (x >> 17);
        x *= 0x1b873593;
        return x;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The States:
//
// A State contains a generator's state as a collection of unsigned words.
// It provides access to those words and to a `step()` method that advances the state to the next iteration.
// --------------------------------------------------------------------------------------------------------------------

/// @brief  The state class for the @c xoshiro family of pseudorandom generators.
/// @tparam N, T The state is stored as @c N words of some unsigned integer type @c T
/// @tparam A, B These are the parameters used in the @c step() method that advances the state.
template<std::size_t N, std::unsigned_integral T, std::uint8_t A, std::uint8_t B>
class xoshiro {
public:
    /// @brief The type of the words of state
    using word_type = T;

    /// @brief Returns the number of words in the underlying state.
    static constexpr std::size_t word_count() { return N; }

    /// @brief Returns the number of bits in the underlying state.
    static constexpr std::size_t bit_count() { return N * std::numeric_limits<T>::digits; }

    /// @brief Returns a name for this state.
    static constexpr auto xso_name()
    {
        return std::format("xoshiro<{}x{},{},{}>", N, std::numeric_limits<T>::digits, A, B);
    }

    /// @brief Read-only access to the i'th state word.
    constexpr T operator[](std::size_t i) const { return m_state[i]; }

    /// @brief Read-only access to the whole state which we copy into @c dst.
    template<typename Iter>
    constexpr void get_state(Iter dst) const
    {
        std::copy(m_state.cbegin(), m_state.cend(), dst);
    }

    /// @brief Set the state from an iteration of words which shouldn't be all zeros.
    template<typename Iter>
    constexpr void seed(Iter b, Iter e)
    {
        std::copy(b, e, m_state.begin());
    }

    /// @brief Advance the state by one step.
    constexpr void step()
    {
        if constexpr (N == 4) {
            auto tmp = m_state[1] << A;
            m_state[2] ^= m_state[0];
            m_state[3] ^= m_state[1];
            m_state[1] ^= m_state[2];
            m_state[0] ^= m_state[3];
            m_state[2] ^= tmp;
            m_state[3] = std::rotl(m_state[3], B);
        }
        else if constexpr (N == 8) {
            auto tmp = m_state[1] << A;
            m_state[2] ^= m_state[0];
            m_state[5] ^= m_state[1];
            m_state[1] ^= m_state[2];
            m_state[7] ^= m_state[3];
            m_state[3] ^= m_state[4];
            m_state[4] ^= m_state[5];
            m_state[0] ^= m_state[6];
            m_state[6] ^= m_state[7];
            m_state[6] ^= tmp;
            m_state[7] = std::rotl(m_state[7], B);
        }
        else {
            // There is no discernible pattern to the way xoshiro works as the number of words of state increases.
            // The step() method for each N has to be hard coded -- this contrasts to xoroshiro state.
            // So if we get to here we don't have a formula that works to advance the state and need to fail out.
            // Get a useful'ish error message by pumping a deliberately false condition into static_assert(...).
            static_assert(N < 0, "No xoshiro step() implementation for this number of words of state!");
        }
    };

    /// @brief  The state's characteristic polynomial is c(x) = x^n + p(x) where n = digits() and degree[p] < n.
    /// @param  dst We fill this destination with the the precomputed coefficients of @b p(x)
    /// @throws If there are no precomputed characteristic coefficients we throw an error.
    template<typename Iter>
    static constexpr void characteristic_coefficients(Iter dst)
    {
        // In practice we have precomputed the p(x) polynomial for just a few xoshiro's with specific parameters.
        if constexpr (std::is_same_v<T, uint32_t> && N == 4 && A == 9 && B == 11) {
            std::array<T, N> p = {0xde18fc01, 0x1b489db6, 0x6254b1, 0xfc65a2};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else if constexpr (std::is_same_v<T, uint64_t> && N == 4 && A == 17 && B == 45) {
            std::array<T, N> p = {0x9d116f2bb0f0f001, 0x280002bcefd1a5e, 0x4b4edcf26259f85, 0x3c03c3f3ecb19};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else if constexpr (std::is_same_v<T, uint64_t> && N == 8 && A == 11 && B == 21) {
            std::array<T, N> p = {0xcf3cff0c00000001, 0x7fdc78d886f00c63, 0xf05e63fca6d7b781, 0x7a67058e7bbab6f0,
                                  0xf11eef832e32518f, 0x51ba7c47edc758ad, 0x8f2d27268ce4b20b, 0x500055d8b77f};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else {
            throw std::invalid_argument("xoshiro characteristic polynomial not pre-computed for given parameters!");
        }
    }

private:
    std::array<T, N> m_state = {1};
};

/// @brief  The state for the @c xoroshiro family of pseudorandom generators.
/// @tparam N, T    The state is stored as @c N words of some unsigned integer type @c T
/// @tparam A, B, C These are the parameters used in the @c step() method that advances the state.
template<std::size_t N, std::unsigned_integral T, std::uint8_t A, std::uint8_t B, std::uint8_t C>
class xoroshiro {
public:
    /// @brief The type of the words of state
    using word_type = T;

    /// @brief Returns the number of words in the underlying state.
    static constexpr std::size_t word_count() { return N; }

    /// @brief Returns the number of bits in the underlying state.
    static constexpr std::size_t bit_count() { return N * std::numeric_limits<T>::digits; }

    /// @brief Returns a name for this state.
    static constexpr auto xso_name()
    {
        return std::format("xoroshiro<{}x{},{},{},{}>", N, std::numeric_limits<T>::digits, A, B, C);
    }

    /// @brief Read-only access to the i'th state word.
    /// @note  For larger values of N we are using the state array as a ring buffer!
    constexpr T operator[](std::size_t i) const { return m_state[(i + m_final + 1) % N]; }

    /// @brief Read-only access to the whole state which we copy into @c dst.
    /// @note  For larger values of N we are using the state array as a ring buffer which needs to be untangled!
    template<typename Iter>
    constexpr void get_state(Iter dst) const
    {
        // Need to untangle the ring buffer we are using to store the state.
        for (std::size_t i = 0; i < N; ++i, ++dst) *dst = operator[](i);
    }

    /// @brief Set the state from an iteration of words which shouldn't be all zeros.
    template<typename Iter>
    constexpr void seed(Iter b, Iter e)
    {
        std::copy(b, e, m_state.begin());
        m_final = N - 1;
    }

    /// @brief Advance the state by one step.
    constexpr void step()
    {
        // Depending on the word_count of N we either do an explicit or implicit array shuffle of the state array.
        if constexpr (N == 2)
            simple_step();
        else
            clever_step();
    }

    /// @brief  The state's characteristic polynomial is c(x) = x^n + p(x) where n = digits() and degree[p] < n.
    /// @param  dst We fill this destination with the the precomputed coefficients of @b p(x)
    /// @throws If there are no precomputed characteristic coefficients we throw an error.
    template<typename Iter>
    static constexpr void characteristic_coefficients(Iter dst)
    {
        if constexpr (std::is_same_v<T, uint32_t> && N == 2 && A == 26 && B == 9 && C == 13) {
            std::array<T, N> p = {0x6e2286c1, 0x53be9da};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else if constexpr (std::is_same_v<T, uint64_t> && N == 2 && A == 24 && B == 16 && C == 37) {
            std::array<T, N> p = {0x95b8f76579aa001, 0x8828e513b43d5};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else if constexpr (std::is_same_v<T, uint64_t> && N == 2 && A == 49 && B == 21 && C == 28) {
            std::array<T, N> p = {0x8dae70779760b081, 0x31bcf2f855d6e5};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else if constexpr (std::is_same_v<T, uint64_t> && N == 16 && A == 25 && B == 27 && C == 36) {
            std::array<T, N> p = {0x5cfeb8cc48ddb211, 0xb73e379d035a06dd, 0x17d5100a20a0350e, 0x7550223f68f98cac,
                                  0x29d373b5c5ed3459, 0x3689b412ef70de48, 0xa1d3b6ee079a7cc6, 0x9bf0b669abd100f8,
                                  0x955c84e105f60997, 0x6ca140c61889cddd, 0xabaf68c5fc3a0e4a, 0xa46134526b83adc5,
                                  0x710704d05683d63,  0x580d080b44b606a2, 0x8040a0580158a1,   0x800081};
            std::copy(p.cbegin(), p.cend(), dst);
        }
        else {
            throw std::invalid_argument("xoroshiro characteristic polynomial not pre-computed for given parameters!");
        }
    }

private:
    std::array<T, N> m_state = {1};   // The state is an array of words -- should never be all zeros!
    std::size_t      m_final = N - 1; // Current location of the final word of state.

    /// @brief Step the state forward using a straight-forward move all the state words approach.
    /// @note  This is an alternative to the clever_step() and is used for small values of @c N.
    constexpr void simple_step()
    {
        // Capture the current values in the first and final words of state
        T s0 = m_state[0];
        T s1 = m_state[N - 1];

        // Shift most of the words of state down one slot
        // Note: It could help to unroll this loop at least once for larger N but the shuffle indices method is
        // better
        for (std::size_t i = 0; i < N - 2; ++i) m_state[i] = m_state[i + 1];

        // Update the first and final words of state
        s1 ^= s0;
        m_state[N - 2] = std::rotl(s0, A) ^ (s1 << B) ^ s1;
        m_state[N - 1] = std::rotl(s1, C);
    }

    /// @brief Step the state forward where we shuffle array indices instead of the state words.
    /// @note  This is an alternative to the simple_step() and is used for larger values of @c N.
    constexpr void clever_step()
    {
        // Which indices point to the current final & first words of state
        std::size_t i_final = m_final;
        std::size_t i_first = (m_final + 1) % N;

        // Capture the current values in the final & first words of state
        T s_final = m_state[i_final];
        T s_first = m_state[i_first];

        // Update the values for the final & first words of state
        s_final ^= s_first;
        m_state[i_final] = std::rotl(s_first, A) ^ (s_final << B) ^ s_final;
        m_state[i_first] = std::rotl(s_final, C);

        // Step the index of the final word of state -- this shuffles the state array down a slot.
        m_final = i_first;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The Scramblers:
//
// A Scrambler is a functor that is passed a State and returns a single unsigned output word.
// --------------------------------------------------------------------------------------------------------------------

/// @brief  The "*" scrambler returns a simple multiple of one of the state words.
/// @tparam S The multiplier.
/// @tparam w Index of the state word in question.
template<auto S, std::size_t w>
struct star {
    /// @brief Reduces the state input to a single word of output.
    constexpr auto operator()(const auto& state) const { return state[w] * S; }

    /// @brief Returns a name for this scrambler.
    static constexpr auto xso_name() { return std::format("star<{:x},{}>", S, w); }
};

/// @brief  The "**" scrambler returns a scrambled version of one of the state words.
/// @tparam S, R, T Parameters in the scramble method.
/// @tparam w Index of the state word being in question.
template<auto S, auto R, auto T, std::size_t w>
struct star_star {
    /// @brief Reduces the state input to a single word of output.
    constexpr auto operator()(const auto& state) const { return std::rotl(state[w] * S, R) * T; }

    /// @brief Returns a name for this scrambler.
    static constexpr auto xso_name() { return std::format("star_star<{:x},{},{}>", S, R, w); }
};

/// @brief  The "+" scrambler returns the sum of two of the state words.
/// @tparam w0, w1 Indices of the two state words in question.
template<std::size_t w0, std::size_t w1>
struct plus {
    /// @brief Reduces the state input to a single word of output.
    constexpr auto operator()(const auto& state) const { return state[w0] + state[w1]; }

    /// @brief Returns a name for this scrambler.
    static constexpr auto xso_name() { return std::format("plus<{},{}>", w0, w1); }
};

/// @brief  The "++" scrambler returns a scrambled version of two of the state words.
/// @tparam R Parameter in the scramble method.
/// @tparam w0, w1 Indices of the two state words in question.
template<auto R, std::size_t w0, std::size_t w1>
struct plus_plus {
    /// @brief Reduces the state input to a single word of output.
    constexpr auto operator()(const auto& state) const { return std::rotl(state[w0] + state[w1], R) + state[w0]; }

    /// @brief Returns a name for this scrambler.
    static constexpr auto xso_name() { return std::format("plus_plus<{},{},{}>", R, w0, w1); }
};

// --------------------------------------------------------------------------------------------------------------------
// Type aliases for all 17 generators talked about in the Black & Vigna paper
// --------------------------------------------------------------------------------------------------------------------

// clang-format off
// First we define the preferred versions of the xoshiro engines with specific choices for A & B.
using xoshiro_4x32              = xoshiro<4, uint32_t, 9,  11>;
using xoshiro_4x64              = xoshiro<4, uint64_t, 17, 45>;
using xoshiro_8x64              = xoshiro<8, uint64_t, 11, 21>;

// And the preferred versions of the xoshiro engines with specific choices for A, B & C.
using xoroshiro_2x32            = xoroshiro<2,  uint32_t, 26, 9,  13>;
using xoroshiro_2x64            = xoroshiro<2,  uint64_t, 24, 16, 37>;
using xoroshiro_2x64b           = xoroshiro<2,  uint64_t, 49, 21, 28>;  // Alternative for 2x64 case
using xoroshiro_16x64           = xoroshiro<16, uint64_t, 25, 27, 36>;

// Preferred/analyzed versions of the xoshiro PRNG's
using xoshiro_4x32_plus         = generator<xoshiro_4x32, plus<0, 3>>;
using xoshiro_4x32_plus_plus    = generator<xoshiro_4x32, plus_plus<7, 0, 3>>;
using xoshiro_4x32_star_star    = generator<xoshiro_4x32, star_star<5, 7, 9, 1>>;
using xoshiro_4x64_plus         = generator<xoshiro_4x64, plus<0, 3>>;
using xoshiro_4x64_plus_plus    = generator<xoshiro_4x64, plus_plus<23, 0, 3>>;
using xoshiro_4x64_star_star    = generator<xoshiro_4x64, star_star<5, 7, 9, 1>>;
using xoshiro_8x64_plus         = generator<xoshiro_8x64, plus<2, 0>>;
using xoshiro_8x64_plus_plus    = generator<xoshiro_8x64, plus_plus<17, 2, 0>>;
using xoshiro_8x64_star_star    = generator<xoshiro_8x64, star_star<5, 7, 9, 1>>;

// Preferred/analyzed versions of the xoroshiro PRNG's
using xoroshiro_2x32_star       = generator<xoroshiro_2x32,  star<0x9E3779BB, 0>>;
using xoroshiro_2x32_star_star  = generator<xoroshiro_2x32,  star_star<0x9E3779BBu, 5, 5, 0>>;
using xoroshiro_2x64_plus       = generator<xoroshiro_2x64,  plus<0, 1>>;
using xoroshiro_2x64_plus_plus  = generator<xoroshiro_2x64b, plus_plus<17, 0, 1>>;
using xoroshiro_2x64_star_star  = generator<xoroshiro_2x64,  star_star<5, 7, 9, 0>>;
using xoroshiro_16x64_plus_plus = generator<xoroshiro_16x64, plus_plus<23, 15, 0>>;
using xoroshiro_16x64_star      = generator<xoroshiro_16x64, star<0x9e3779b97f4a7c13, 0>>;
using xoroshiro_16x64_star_star = generator<xoroshiro_16x64, star_star<5, 7, 9, 0>>;
// clang-format on

/// @brief The "default" 32-bit output generator -- used as @c xso::rng32
using rng32 = xoshiro_4x32_star_star;

/// @brief The "default" 64-bit output generator -- used as @c xso::rng64
using rng64 = xoshiro_4x64_star_star;

/// @brief The "overall default" generator -- used as @c xso::rng
using rng = rng64;

} // namespace xso

// --------------------------------------------------------------------------------------------------------------------
// Non-member functions that are used to "jump" a generator or state far ahead in its stream.
// --------------------------------------------------------------------------------------------------------------------
namespace xso::internal {

// Polynomial Reduction:
// Start with internal functions that are used to compute x^J mod c(x) where c(x) = x^n + p(x) and degree[p] < n.
// Re-implements the more general `bit::polynomial::reduce` method -- see https://nessan.gitbub.io/bit.
// Repeated here to make `xoshiro/xoroshiro` complete w/o any need to reference the `bit` library.

/// @brief  Riffle a word into two other words containing the bits from @c src interleaved with zeros.
/// @return With an 8-bit word @c src = `abcdefgh`, on return @c lo = `a0b0c0d0 and @c hi = `e0f0g0h0`.
template<std::unsigned_integral word_type>
constexpr void
riffle(word_type src, word_type& lo, word_type& hi)
{
    // Constants
    constexpr std::size_t bits_per_word = std::numeric_limits<word_type>::digits;
    constexpr std::size_t half_bits = bits_per_word / 2;
    constexpr word_type   one{1};
    constexpr word_type   ones = std::numeric_limits<word_type>::max();

    // Split the src into lo and hi halves.
    lo = src & (ones >> half_bits);
    hi = src >> half_bits;

    // Some magic to interleave the respective halves with zeros.
    for (auto i = bits_per_word / 4; i > 0; i /= 2) {
        word_type div = word_type(one << i) | one;
        word_type msk = ones / div;
        lo = (lo ^ (lo << i)) & msk;
        hi = (hi ^ (hi << i)) & msk;
    }
}

/// @brief  Riffle an array of unsigneds into two others which will get the bits from @c src interleaved with zeros.
/// @return We treat @c [lo|hi] as contiguous storage and fill the elements of @c lo first and then @c hi
/// @note   You can reuse @c src for the output array @c lo -- the call @c riffle(src,src,hi) will work fine.
template<std::unsigned_integral word_type, std::size_t N>
constexpr void
riffle(const std::array<word_type, N>& src, std::array<word_type, N>& lo, std::array<word_type, N>& hi)
{
    // We will riffle each word in src into two other words x & y
    word_type x, y;

    // We work through src in reverse order -- this allows the reuse of src for lo!
    // Treating [lo|hi] as contiguous storage, first working back through hi and then back through lo.
    for (std::size_t i = N; i-- > 0;) {
        riffle(src[i], x, y);
        if (2 * i + 1 > N) {
            // Both x & y go in hi -- note that if 2i + 1 - N > 0 then 2i - N is >= 0.
            hi[2 * i - N] = x;
            hi[2 * i + 1 - N] = y;
        }
        else if (2 * i + 1 == N) {
            // Straddling situation where y goes in the first word of hi and x in the last word of lo.
            lo[N - 1] = x;
            hi[0] = y;
        }
        else {
            // Need to pop both x & y into the lo array.
            lo[2 * i] = x;
            lo[2 * i + 1] = y;
        }
    }
}

/// @brief  Computes x^e mod c(x) in GF(2) for e = J or 2^J and c(x) = x^n + p(x) where degree[p] < n.
/// @param  p We are passed the coefficients of @b p(x) packed into an an array.
/// @param  J_is_pow2 If true we compute x^(2^J) mod c(x) -- allows e.g. e = 2^100 which overflows a @c std::size_t
/// @return We return the coefficients of r(x) = x^e mod c(x) in the same type of array as @c p.
/// @note   linters will (reasonably) complain that the complexity of this method is rather high!
template<std::unsigned_integral T, std::size_t N>
constexpr std::array<T, N>
reduce(const std::array<T, N>& p, std::size_t J, bool J_is_pow2)
{
    // This is similar in spirit/algorithm to `bit::polynomial::reduce` method -- see https://nessan.github.io/bit
    // This version is less general as it assumes that the degree of c(x) is a multiple of 32 + 1.
    // NOTE: The `bit` version is more factored and more readable (this method's complexity is high).

    // Constant we use to indicate "no such position"/"not found" and a couple of others.
    constexpr auto npos = static_cast<std::size_t>(-1);
    constexpr T    one{1};

    // The polynomial c(x) = x^n + p(x) where p(x) = p_0 + p_1 x + ... + p_{n-1} x^{n-1} and n is:
    constexpr std::size_t bits_per_word = std::numeric_limits<T>::digits;
    constexpr std::size_t n = N * bits_per_word;

    // lambda: Returns the index of the word that holds p_i.
    auto word = [=](std::size_t i) { return i / bits_per_word; };

    // lambda: Returns the bit location of p_i inside the word that holds it.
    auto bit = [=](std::size_t i) { return i % bits_per_word; };

    // lambda: Returns a mask that isolates p_i within the word that holds it.
    auto mask = [=](std::size_t i) { return T{one << bit(i)}; };

    // lambda: Returns true if poly_i is 1.
    auto test = [=](const auto& poly, std::size_t i) -> bool { return poly[word(i)] & mask(i); };

    // lambda: Sets poly_i to 1.
    auto set = [=](auto& poly, std::size_t i) { poly[word(i)] |= mask(i); };

    // lambda: Returns the index for the least significant set bit in the argument or `npos` if none set.
    auto lsb = [=](T w) { return w == 0 ? npos : static_cast<std::size_t>(std::countr_zero(w)); };

    // lambda: Returns the index for the most significant set bit in the argument or `npos` if none set.
    auto msb = [=](T w) { return static_cast<std::size_t>(std::bit_width(w) - 1); };

    // lambda: Returns the first set coefficient in poly or `npos` if the coefficients are all zero.
    auto first_set = [=](const auto& poly) {
        for (std::size_t i = 0; i < N; ++i)
            if (poly[i] != 0) return i * bits_per_word + lsb(poly[i]);
        return npos;
    };

    // lambda: Returns the final set coefficient in poly or `npos` if the coefficients are all zero.
    auto final_set = [=](const auto& poly) {
        std::size_t i = N;
        while (i--)
            if (poly[i] != 0) return i * bits_per_word + msb(poly[i]);
        return npos;
    };

    // lambda: Returns true if the highest coefficient in `poly` is set (then poly is said to be monic).
    auto monic = [=](const auto& poly) {
        constexpr std::size_t complement = bits_per_word - 1;
        constexpr T           final_bit_mask = T{one << complement};
        return poly[N - 1] & final_bit_mask;
    };

    // lambda: Computes lhs <- lhs + rhs which in GF(2) is equivalent to  lhs <- lhs^rhs.
    auto add = [=](auto& lhs, const auto& rhs) {
        for (std::size_t i = 0; i < N; ++i) lhs[i] ^= rhs[i];
    };

    // lambda: Performs a one place shift on the polynomial coefficients stored poly.
    // Shift is to the the right if you think the elements are in vector order [p0,p1,p2,p3] -> [0,p0,p1,p2].
    // Shift is to the left when you think in bit order [b3,b2,b1,b0] -> [b2,b1,b0,0].
    auto shift = [=](auto& poly) {
        constexpr std::size_t complement = bits_per_word - 1;
        for (std::size_t i = N - 1; i > 0; --i) {
            auto l = static_cast<T>(poly[i] << 1);
            auto r = static_cast<T>(poly[i - 1] >> complement);
            poly[i] = l | r;
        }
        poly[0] <<= 1;
    };

    // lambda: If degree[poly] < n, this performs poly(x) <- x*poly(x) mod c(x) where c(x) = x^n + p(x).
    auto times_x_step = [&](auto& poly) {
        bool add_p = monic(poly);
        shift(poly);
        if (add_p) add(poly, p);
    };

    // We precompute x^{n + i} mod c(x) for i = 0, ..., n-1 starting from the known x^n mod c(x) = p.
    // Each x^{n + i} mod c(x) is a word-vector of coefficients and we put the lot into a std::vector.
    using array_type = std::array<T, N>;
    std::vector<array_type> power_mod(n);
    power_mod[0] = p;
    for (std::size_t i = 1; i < n; ++i) {
        power_mod[i] = power_mod[i - 1];
        times_x_step(power_mod[i]);
    }

    // Some work space we use below.
    array_type hi;

    // lambda: If degree[poly] < n, performs: poly(x) <- poly(x)^2 mod c(x) where as usual c(x) = x^n + p(x).
    auto square_step = [&](auto& poly) {
        // Compute poly(x)^2 -- in GF(2) this means interspersing all the coefficients with zeros.
        // We actually riffle poly directly into two arrays lo & hi so that poly(x)^2 = lo(x) + x^n hi(x).
        // This only works because we assume n some multiple of N (i.e. all bits in poly matter).
        // NOTE: Our riffle method above for arrays lets us reuse the poly array for lo.
        riffle(poly, poly, hi);

        // poly(x)^2 mod c(x) now is poly(x) + x^n hi(x) mod c(x) as degree[poly] < n.
        // Add the x^n hi(x) mod c(x) term-by-term noting that at most every second term in hi(x) is 1.
        auto hi_first = first_set(hi);
        if (hi_first != npos) {
            auto hi_final = final_set(hi);
            for (std::size_t i = hi_first; i <= hi_final; i += 2)
                if (test(hi, i)) add(poly, power_mod[i]);
        }
    };

    // Initialize our return value to all zeros.
    array_type r;
    for (std::size_t i = 0; i < N; ++i) r[i] = 0;

    // Case: e = 2^J -- we just do J squaring steps starting from r(x) = x to get to  x^(2^J) mod c(x).
    if (J_is_pow2) {
        set(r, 1);
        for (std::size_t j = 0; j < J; ++j) square_step(r);
        return r;
    }

    // Case e = J < n: Then x^J mod c(x) = x^J so we can set the appropriate coefficient in r and return.
    if (J < n) {
        set(r, J);
        return r;
    }

    // Case e = J = n: Then x^J mod c(x) = p(x).
    if (J == n) return p;

    // Case e = J > n: We use a square & multiply algorithm:
    // Note that if e.g. J = 0b00010111 then std::bit_floor(J) = 0b00010000.
    std::size_t J_bit = std::bit_floor(J);

    // Start with r(x) = x mod c(x) which takes care of the most significant binary digit in J.
    set(r, 1);
    J_bit >>= 1;

    // And off we go ...
    while (J_bit) {

        // Always do a square step and then a times_x step if necessary (i.e. if current bit in J is set).
        square_step(r);
        if (J & J_bit) times_x_step(r);

        // On to the next bit position in n.
        J_bit >>= 1;
    }

    return r;
}

} // namespace xso::internal

namespace xso {

/// @brief  Returns the coefficients for the jump polynomial that jumps the generator/state ahead by N or 2^N steps.
/// @param  N_is_pow2 If true the jump size is 2^N -- allows for e.g. 2^100 slots which overflows a @c std::size_t
/// @return If the associated characteristic polynomial is c(x) the jump polynomial is: r(x) = x^N mod c(x).
///         We return the coefficients of @b r(x) in an appropriate sized @c std::array
/// @throw  If the @c State has no precomputed characteristic coefficients this fails.
template<typename State>
std::array<typename State::word_type, State::word_count()>
jump_coefficients(std::size_t N, bool N_is_pow2 = false)
{
    // Retrieve the coefficients of p(x) from the State.
    std::array<typename State::word_type, State::word_count()> p;
    State::characteristic_coefficients(p.begin());

    // The jump polynomial is x^J mod c(x) -- that computation expects to be handed p(x) where c(x) = x^n + p(x).
    return internal::reduce(p, N, N_is_pow2);
}

/// @brief  Returns the coefficients for the jump polynomial that jumps the generator/state ahead by N or 2^N steps.
/// @param  N_is_pow2 If true the jump size is 2^N -- allows for e.g. 2^100 slots which overflows a @c std::size_t
/// @return If the state's characteristic polynomial is c(x) the jump polynomial is: r(x) = x^N mod c(x).
///         We return the coefficients of r(x) in an appropriate sized @c std::array
/// @note   This version allows calls like @c xso::jump_coefficients(rng,N)
/// @throw  If the @c State has no precomputed characteristic coefficients this fails.
template<typename State>
std::array<typename State::word_type, State::word_count()>
jump_coefficients(const State&, std::size_t N, bool N_is_pow2 = false)
{
    // The first argument is only used to determine the `State` type.
    return jump_coefficients<State>(N, N_is_pow2);
}

/// @brief Jumps state quickly forward by J steps where J can be huge (e.g. 2^100)
/// @param state      The generator or state that needs to be jumped.
/// @param jump_coeff The coefficients of the jump polynomial x^J mod c(x) packed in an array of unsigned words.
/// @note  You get the @c jump_coeff array by first calling the @c jump_coefficients method for the jump in question.
template<typename State>
void
jump(State& state, const std::array<typename State::word_type, State::word_count()>& jump_coeff)
{
    using word_type = typename State::word_type;
    constexpr std::size_t word_count = State::word_count();

    // Some work space ...
    std::array<word_type, word_count> sum;

    // Some help for iterating through the set bits in each word of r.
    std::size_t bits_per_word = std::numeric_limits<word_type>::digits;
    word_type   one{1};

    // Computing the sum r(T).s = r_0.s + r_1.s^1 + ... + r_{n-1} s^{n-1} where s is the current state.
    // T is the state's transition matrix and we compute s^{i+1} = T.s^i using the step() method.
    sum.fill(0);
    for (std::size_t i = 0; i < word_count; ++i) {

        // Iterate over the bits in the current r(x) word
        word_type ji = jump_coeff[i];
        for (std::size_t b = 0; b < bits_per_word; ++b) {

            // If this bit in r is set then we add the current state to the sum.
            if (ji & static_cast<word_type>(one << b)) {
                for (std::size_t w = 0; w < word_count; ++w) sum[w] ^= state[w];
            }

            // Compute the next state s^{i+1} by calling the step() method.
            state.step();
        }
    }

    // Store the computed jump back into the state
    state.seed(sum.cbegin(), sum.cend());
}

// --------------------------------------------------------------------------------------------------------------------
// Partitions of random number stream:
//
// For parallel processing applications it can be useful to split a single random number stream into a number of
// non-overlapping "partitions". Then different computational threads can get their "own set" of random numbers without
// worrying about stream overlaps etc.
//
// The partitions need to be very large so this only works for States where we can jump far ahead in an efficient way.
// --------------------------------------------------------------------------------------------------------------------

/// @brief  Partition a random number stream into a number of non-overlapping sub-streams.
/// @tparam State Any state/generator type that we can @c jump ahead in its stream.
template<typename State>
class partition {
public:
    /// @brief Constrict a partition for the passed generator/state.
    /// @param state        The parent generator/state already seeded to some starting point for its state.
    /// @param n_partitions The number of non-overlapping partitions we will split the parent state stream into.
    partition(const State& state, std::size_t n_partitions) : m_state{state}
    {
        // Make sure the requested number of partitions makes sense -- silently fix any issues.
        if (n_partitions == 0) n_partitions = 1;

        // How many bits of state in State?
        auto n_bits = State::bit_count();

        // The period of the generator is 2^n_bits so each partition ideally has size 2^n_bits / n_partitions. That
        // number will probably overflow a std::size_t so we must keep everything in log 2 form. First we find the
        // smallest n such that 2^n >= n_partitions - 1.
        // Note if n_partitions is 128 the following gives n = 7 and does the same if n = 100.
        auto n = static_cast<std::size_t>(std::bit_width(n_partitions - 1));

        // We will create 2^n partitions which is probably more than needed but the wastage is negligible.
        // To create those 2^n partitions we must be able to jump ahead 2^(n_bits - n) steps many times.
        auto power_2 = n_bits - n;

        // Precompute the jump coefficients to advance the generator 2^power_2 steps i.e. along to the next partition.
        m_jump_coefficients = jump_coefficients<State>(power_2, true);
    }

    /// @brief  Get the next sub-stream.
    /// @return A new generator seeded at the start of the next sub-stream of the parent random number
    /// stream.
    State next()
    {
        // We already have a pre-baked generator seeded at the right spot ready to go.
        State retval = m_state;

        // Prep for the next call by jumping the parent copy once more using our precomputed jump
        // coefficients.
        jump(m_state, m_jump_coefficients);

        // And return the pre-baked one ...
        return retval;
    }

private:
    using array_type = std::array<typename State::word_type, State::word_count()>;
    State      m_state;
    array_type m_jump_coefficients;
};

} // namespace xso

// --------------------------------------------------------------------------------------------------------------------
// Some extra functionality that depends on the `bit` library ...
// --------------------------------------------------------------------------------------------------------------------
#ifdef BIT

// clang-format off
#include <bit/bit.h>
// clang-format on
namespace xso {

/// @brief Returns the transition matrix for a state/generator type as a @c bit::matrix.
template<typename State>
auto
transition_matrix()
{
    // The state bits are packed into words of this type (in practice, 32 or 64 bit unsigneds).
    using word_type = typename State::word_type;

    // The number of words & bits of state.
    constexpr std::size_t n_words = State::word_count();
    constexpr std::size_t n_bits = State::bit_count();

    /// Some bit types we use.
    using matrix_type = bit::matrix<>;
    using vector_type = bit::vector<>;

    // The transition matrix will be a square n_bits x n_bits matrix over GF(2).
    matrix_type retval{n_bits, n_bits};

    // Some work-space in word and bit space
    using array_type = std::array<word_type, n_words>;
    array_type  words;
    vector_type bits{n_bits};

    // Create an state instance (this can be a State or a full generator -- just needs to support a few methods).
    State state;

    // We get the columns of the transition matrix by looking  at the action of step() on all the unit states.
    for (std::size_t k = 0; k < n_bits; ++k) {

        // Create the k'th unit state (i.e. the state just has the k'th bit set and all others are zero)
        bits.reset();
        bits.set(k);

        // Seed the state from that k'th unit state -- first translating the bits to words.
        bits.export_bits(words);
        state.seed(words.cbegin(), words.cend());

        // Advance that k'th unit state one step.
        state.step();

        // Grab the resulting state as an array of words and convert that to a bit-vector
        state.get_state(words.begin());
        bits.import_bits(words);

        // Store those bits into column k of the transition matrix.
        // Note that columnar access for a bit::matrix must be done element by element.
        for (std::size_t i = 0; i < n_bits; ++i) retval(i, k) = bits[i];
    }

    return retval;
}

/// @brief Returns the transition matrix for a state/generator type as a @c bit::matrix.
/// @note  This version allows calls like @c xso::transition_matrix(rng)
template<typename State>
auto
transition_matrix(const State&)
{
    // The argument is only used to determine the `State` type.
    return transition_matrix<State>();
}

/// @brief Returns the characteristic polynomial for a state/generator type as a @c bit::polynomial.
/// @note  If the transition matrix is n x n then the return will have word_count n+1 and should be monic.
template<typename State>
auto
characteristic_polynomial()
{
    auto T = transition_matrix<State>();
    return bit::characteristic_polynomial(T);
}

/// @brief Returns the characteristic polynomial for a state/generator type as a @c bit::polynomial.
/// @note  This version allows calls like @c xso::characteristic_polynomial(rng)
template<typename State>
auto
characteristic_polynomial(const State&)
{
    // The argument is only used to determine the `State` type.
    return characteristic_polynomial<State>();
}

/// @brief  Returns a jump polynomial that moves a state/generator type @c J steps ahead in its random number stream.
/// @param  c The precomputed characteristic polynomial for a state/generator type as a @c bit::polynomial
/// @param  N We want to jump by J = N steps or J = 2^N steps (for really huge jumps).
/// @param  N_is_pow2 If true we want to jump by 2^N steps -- allows for say J = 2^100 which overflows normal ints.
/// @return Returns the jump polynomial x^J mod c(x) as a @c bit::polynomial
template<std::unsigned_integral Block, typename Allocator>
auto
jump_polynomial(const bit::polynomial<Block, Allocator>& c, std::size_t N, bool N_is_pow2 = false)
{
    // The bit-polynomial class has a method to compute x^J mod c(x).
    return c.reduce(N, N_is_pow2);
}

/// @brief Jumps a state/generator ahead in its random number stream by @c J steps.
/// @param jump_poly The precomputed bit-polynomial x^J mod c(x) where c(x) is the characteristic polynomial.
/// @note  You get @c jump_poly by first calling the @c jump_polynomial method for the jump in question.
template<typename State, std::unsigned_integral Block, typename Allocator>
void
jump(State& state, const bit::polynomial<Block, Allocator>& jump_poly)
{
    std::array<typename State::word_type, State::word_count()> sum;

    // Computing [r_0 + r_1 T + ... + r_{m-1} T^{m-1}].s where s is the current state and r is the jump polynomial.
    // T is the state's transition matrix so we can compute s^{i+1} = T.s^i using the step() method.
    sum.fill(0);
    for (std::size_t i = 0; i < jump_poly.size(); ++i) {
        if (jump_poly[i])
            for (std::size_t w = 0; w < State::word_count(); ++w) sum[w] ^= state[w];
        state.step();
    }

    // Perform the computed jump by reseeding the state from the computed sum ...
    state.seed(sum.cbegin(), sum.cend());
}

} // namespace xso

#endif // BIT

/// @brief A concept that matches any type that has an accessible `xso_name()` class `method.
template<typename T>
concept has_xso_name_class_method = requires {
    { T::xso_name() } -> std::convertible_to<std::string>;
};

/// @brief Connect our classes to @c std::format and friends by specializing the @c std:formatter struct.
/// @note  This uses the fact that our classes have a class method @c xso_name() that returns a string.
/// @note  Specializations of @c std::formatter are always in the @c std namespace.
template<has_xso_name_class_method T>
struct std::formatter<T> {

    /// @brief Parse the format specifier -- currently only handle the default empty specifier
    constexpr auto parse(const std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        assert(it == ctx.end() || *it == '}');
        return it;
    }

    /// @brief Push out a formatted xso::generator using its @c xso_name(...) method.
    template<class FormatContext>
    auto format(const T&, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", T::xso_name());
    }
};

/// @brief The usual output stream operator for an xso::generator, State, or Scrambler.
template<has_xso_name_class_method T>
std::ostream&
operator<<(std::ostream& s, const T&)
{
    s << T::xso_name();
    return s;
}