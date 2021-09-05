/* File: z-rand.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: a simple random number generator -BEN- */

#if defined(WINDOWS)
#include <Windows.h>
#endif

#include "term/z-rand.h"

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

/*
 * Currently unused
 */
uint16_t Rand_place;

/*
 * Current "state" table for the RNG
 * Only index 0 to 3 are used
 */
uint32_t Rand_state[RAND_DEG] = {
    123456789,
    362436069,
    521288629,
    88675123,
};

static uint32_t u32b_rotl(const uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

/*
 * Initialize RNG state
 */
static void Rand_seed(uint32_t seed, uint32_t *state)
{
    int i;

    for (i = 1; i <= 4; ++i) {
        seed = 1812433253UL * (seed ^ (seed >> 30)) + i;
        state[i - 1] = seed;
    }
}

/*
 * Xoshiro128** Algorithm
 */
static uint32_t Rand_Xoshiro128starstar(uint32_t *state)
{
    const uint32_t result = u32b_rotl(state[1] * 5, 7) * 9;

    const uint32_t t = state[1] << 9;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = u32b_rotl(state[3], 11);

    return result;
}

static const uint32_t Rand_Xorshift_max = 0xFFFFFFFF;

/*
 * Initialize the RNG using a new seed
 */
void Rand_state_set(uint32_t seed) { Rand_seed(seed, Rand_state); }

void Rand_state_init(void)
{
#ifdef RNG_DEVICE

    FILE *fp = fopen(RNG_DEVICE, "r");
    int n;

    do {
        n = fread(Rand_state, sizeof(Rand_state[0]), 4, fp);
    } while (n != 4 || (Rand_state[0] | Rand_state[1] | Rand_state[2] | Rand_state[3]) == 0);

    fclose(fp);

#elif defined(WINDOWS)

    HCRYPTPROV hProvider;

    CryptAcquireContext(&hProvider, nullptr, nullptr, PROV_RSA_FULL, 0);

    do {
        CryptGenRandom(hProvider, sizeof(Rand_state[0]) * 4, (BYTE *)Rand_state);
    } while ((Rand_state[0] | Rand_state[1] | Rand_state[2] | Rand_state[3]) == 0);

    CryptReleaseContext(hProvider, 0);

#else

    /* Basic seed */
    uint32_t seed = (time(nullptr));
#ifdef SET_UID
    /* Mutate the seed on Unix machines */
    seed = ((seed >> 3) * (getpid() << 1));
#endif
    /* Seed the RNG */
    Rand_state_set(seed);

#endif
}

/*
 * Backup the RNG state
 */
void Rand_state_backup(uint32_t *backup_state)
{
    int i;

    for (i = 0; i < 4; ++i) {
        backup_state[i] = Rand_state[i];
    }
}

/*
 * Restore the RNG state
 */
void Rand_state_restore(uint32_t *backup_state)
{
    int i;

    for (i = 0; i < 4; ++i) {
        Rand_state[i] = backup_state[i];
    }
}

/*
 * Extract a "random" number from 0 to m-1, via "ENERGY_DIVISION"
 */
static int32_t Rand_div_impl(int32_t m, uint32_t *state)
{
    uint32_t scaling;
    uint32_t past;
    uint32_t ret;

    /* Hack -- simple case */
    if (m <= 1)
        return 0;

    scaling = Rand_Xorshift_max / m;
    past = scaling * m;

    do {
        ret = Rand_Xoshiro128starstar(state);
    } while (ret >= past);

    return ret / scaling;
}

int32_t Rand_div(int32_t m) { return Rand_div_impl(m, Rand_state); }

/*
 * The number of entries in the "randnor_table"
 */
#define RANDNOR_NUM 256

/*
 * The standard deviation of the "randnor_table"
 */
#define RANDNOR_STD 64

/*
 * The normal distribution table for the "randnor()" function (below)
 */
static int16_t randnor_table[RANDNOR_NUM] =
{
	206,     613,    1022,    1430,		1838,	 2245,	  2652,	   3058,
	3463,    3867,    4271,    4673,	5075,	 5475,	  5874,	   6271,
	6667,    7061,    7454,    7845,	8234,	 8621,	  9006,	   9389,
	9770,   10148,   10524,   10898,   11269,	11638,	 12004,	  12367,
	12727,   13085,   13440,   13792,   14140,	14486,	 14828,	  15168,
	15504,   15836,   16166,   16492,   16814,	17133,	 17449,	  17761,
	18069,   18374,   18675,   18972,   19266,	19556,	 19842,	  20124,
	20403,   20678,   20949,   21216,   21479,	21738,	 21994,	  22245,

	22493,   22737,   22977,   23213,   23446,	23674,	 23899,	  24120,
	24336,   24550,   24759,   24965,   25166,	25365,	 25559,	  25750,
	25937,   26120,   26300,   26476,   26649,	26818,	 26983,	  27146,
	27304,   27460,   27612,   27760,   27906,	28048,	 28187,	  28323,
	28455,   28585,   28711,   28835,   28955,	29073,	 29188,	  29299,
	29409,   29515,   29619,   29720,   29818,	29914,	 30007,	  30098,
	30186,   30272,   30356,   30437,   30516,	30593,	 30668,	  30740,
	30810,   30879,   30945,   31010,   31072,	31133,	 31192,	  31249,

	31304,   31358,   31410,   31460,   31509,	31556,	 31601,	  31646,
	31688,   31730,   31770,   31808,   31846,	31882,	 31917,	  31950,
	31983,   32014,   32044,   32074,   32102,	32129,	 32155,	  32180,
	32205,   32228,   32251,   32273,   32294,	32314,	 32333,	  32352,
	32370,   32387,   32404,   32420,   32435,	32450,	 32464,	  32477,
	32490,   32503,   32515,   32526,   32537,	32548,	 32558,	  32568,
	32577,   32586,   32595,   32603,   32611,	32618,	 32625,	  32632,
	32639,   32645,   32651,   32657,   32662,	32667,	 32672,	  32677,

	32682,   32686,   32690,   32694,   32698,	32702,	 32705,	  32708,
	32711,   32714,   32717,   32720,   32722,	32725,	 32727,	  32729,
	32731,   32733,   32735,   32737,   32739,	32740,	 32742,	  32743,
	32745,   32746,   32747,   32748,   32749,	32750,	 32751,	  32752,
	32753,   32754,   32755,   32756,   32757,	32757,	 32758,	  32758,
	32759,   32760,   32760,   32761,   32761,	32761,	 32762,	  32762,
	32763,   32763,   32763,   32764,   32764,	32764,	 32764,	  32765,
	32765,   32765,   32765,   32766,   32766,	32766,	 32766,	  32767,
};

/*
 * Generate a random integer number of NORMAL distribution
 *
 * The table above is used to generate a pseudo-normal distribution,
 * in a manner which is much faster than calling a transcendental
 * function to calculate a true normal distribution.
 *
 * Basically, entry 64*N in the table above represents the number of
 * times out of 32767 that a random variable with normal distribution
 * will fall within N standard deviations of the mean.  That is, about
 * 68 percent of the time for N=1 and 95 percent of the time for N=2.
 *
 * The table above contains a "faked" final entry which allows us to
 * pretend that all values in a normal distribution are strictly less
 * than four standard deviations away from the mean.  This results in
 * "conservative" distribution of approximately 1/32768 values.
 *
 * Note that the binary search takes up to 16 quick iterations.
 */
int16_t randnor(int mean, int stand)
{
    int16_t tmp;
    int16_t offset;

    int16_t low = 0;
    int16_t high = RANDNOR_NUM;
    if (stand < 1)
        return (int16_t)(mean);

    /* Roll for probability */
    tmp = (int16_t)randint0(32768);

    /* Binary Search */
    while (low < high) {
        int mid = (low + high) >> 1;

        /* Move right if forced */
        if (randnor_table[mid] < tmp) {
            low = mid + 1;
        }

        /* Move left otherwise */
        else {
            high = (int16_t)mid;
        }
    }

    /* Convert the index into an offset */
    offset = (long)stand * (long)low / RANDNOR_STD;

    /* One half should be negative */
    if (randint0(100) < 50)
        return (mean - offset);

    /* One half should be positive */
    return (mean + offset);
}

/*
 * Generates damage for "2d6" style dice rolls
 */
int16_t damroll(DICE_NUMBER num, DICE_SID sides)
{
    int i, sum = 0;
    for (i = 0; i < num; i++)
        sum += randint1(sides);
    return (int16_t)(sum);
}

/*
 * Same as above, but always maximal
 */
int16_t maxroll(DICE_NUMBER num, DICE_SID sides) { return (num * sides); }

/*
 * Given a numerator and a denominator, supply a properly rounded result,
 * using the RNG to smooth out remainders.  -LM-
 */
int32_t div_round(int32_t n, int32_t d)
{
    int32_t tmp;

    /* Refuse to divide by zero */
    if (!d)
        return (n);

    /* Division */
    tmp = n / d;

    /* Rounding */
    if ((ABS(n) % ABS(d)) > randint0(ABS(d))) {
        /* Increase the absolute value */
        if (n * d > 0L)
            tmp += 1L;
        else
            tmp -= 1L;
    }

    /* Return */
    return (tmp);
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
    static bool initialized = false;
    static uint32_t Rand_state_external[4];

    if (!initialized) {
        /* Initialize with new seed */
        uint32_t seed = (uint32_t)time(nullptr);
        Rand_seed(seed, Rand_state_external);
        initialized = true;
    }

    return Rand_div_impl(m, Rand_state_external);
}

bool next_bool() { return randint0(2) == 0; }
