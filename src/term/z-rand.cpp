/* File: z-rand.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: a simple random number generator -BEN- */

#include "term/z-rand.h"
#include "util/rng-xoshiro.h"
#include "world/world.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <random>

/*
 * Angband 2.7.9 introduced a new (optimized) random number generator,
 * based loosely on the old "random.c" from Berkeley but with some major
 * optimizations and algorithm changes.  See below for more details.
 *
 * Code by myself (benh@phial.com) and Randy (randy@stat.tamu.edu).
 *
 * This code provides (1) a "decent" RNG, based on the "BSD-degree-63-RNG"
 * used in Angband 2.7.8, but rather optimized, and (2) a "simple" RNG,
 * based on the simple "LCRNG" currently used in Angband, but "corrected"
 * to give slightly better values.  Both of these are available in two
 * flavors, first, the simple "mod" flavor, which is fast, but slightly
 * biased at high values, and second, the simple "div" flavor, which is
 * less fast (and potentially non-terminating) but which is not biased
 * and is much less subject to low-bit-non-randomness problems.
 *
 * You can select your favorite flavor by proper definition of the
 * "randint0()" macro in the "defines.h" file.
 *
 * Note that, in Angband 2.8.0, the "state" table will be saved in the
 * savefile, so a special "initialization" phase will be necessary.
 *
 * Note the use of the "simple" RNG, first you activate it via
 * "Rand_quick = TRUE" and "Rand_value = seed" and then it is used
 * automatically used instead of the "complex" RNG, and when you are
 * done, you de-activate it via "Rand_quick = FALSE" or choose a new
 * seed via "Rand_value = seed".
 *
 *
 * RNG algorithm was fully rewritten. Upper comment is OLD.
 */

void Rand_state_init(void)
{
    using element_type = Xoshiro128StarStar::state_type::value_type;
    constexpr auto a = std::numeric_limits<element_type>::min();
    constexpr auto b = std::numeric_limits<element_type>::max();

    std::random_device rd;
    std::uniform_int_distribution<element_type> dist(a, b);

    Xoshiro128StarStar::state_type Rand_state{};
    do {
        std::generate(Rand_state.begin(), Rand_state.end(), [&dist, &rd] { return dist(rd); });
    } while (std::all_of(Rand_state.begin(), Rand_state.end(), [](auto s) { return s == 0; }));

    w_ptr->rng.set_state(Rand_state);
}

int rand_range(int a, int b)
{
    if (a > b) {
        return a;
    }
    std::uniform_int_distribution<> d(a, b);
    return d(w_ptr->rng);
}

/*
 * Generate a random integer number of NORMAL distribution
 */
int16_t randnor(int mean, int stand)
{
    if (stand <= 0) {
        return static_cast<int16_t>(mean);
    }
    std::normal_distribution<> d(mean, stand);
    auto result = std::round(d(w_ptr->rng));
    return static_cast<int16_t>(result);
}

/*
 * Generates damage for "2d6" style dice rolls
 */
int16_t damroll(DICE_NUMBER num, DICE_SID sides)
{
    int i, sum = 0;
    for (i = 0; i < num; i++) {
        sum += randint1(sides);
    }
    return (int16_t)(sum);
}

/*
 * Same as above, but always maximal
 */
int16_t maxroll(DICE_NUMBER num, DICE_SID sides)
{
    return num * sides;
}

/*
 * Given a numerator and a denominator, supply a properly rounded result,
 * using the RNG to smooth out remainders.  -LM-
 */
int32_t div_round(int32_t n, int32_t d)
{
    int32_t tmp;

    /* Refuse to divide by zero */
    if (!d) {
        return n;
    }

    /* Division */
    tmp = n / d;

    /* Rounding */
    if ((std::abs(n) % std::abs(d)) > randint0(std::abs(d))) {
        /* Increase the absolute value */
        if (n * d > 0L) {
            tmp += 1L;
        } else {
            tmp -= 1L;
        }
    }

    /* Return */
    return tmp;
}

/*
 * Extract a "random" number from 0 to m-1, using the RNG.
 *
 * This function should be used when generating random numbers in
 * "external" program parts like the main-*.c files.  It preserves
 * the current RNG state to prevent influences on game-play.
 *
 * Could also use rand() from <stdlib.h> directly.
 */
int32_t Rand_external(int32_t m)
{
    if (m <= 0) {
        return 0;
    }

    static std::optional<Xoshiro128StarStar> urbg_external;

    if (!urbg_external.has_value()) {
        /* Initialize with new seed */
        auto seed = static_cast<uint32_t>(time(nullptr));
        urbg_external = Xoshiro128StarStar(seed);
    }

    std::uniform_int_distribution<> d(0, m - 1);
    return d(urbg_external.value());
}
