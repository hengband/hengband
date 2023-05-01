/* File: z-util.c */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Low level utilities -BEN- */

#include "term/z-util.h"

/*
 * Convenient storage of the program name
 */
concptr argv0 = nullptr;

/*
 * Determine if string "t" is equal to string "t"
 */
bool streq(std::string_view a, std::string_view b)
{
    return a == b;
}

/*
 * Determine if string "t" is a suffix of string "s"
 */
bool suffix(std::string_view s, std::string_view t)
{
    //! @todo C++20 では ends_with が使用可能
    if (t.size() > s.size()) {
        return false;
    }

    return s.compare(s.size() - t.size(), s.npos, t) == 0;
}

/*
 * Determine if string "t" is a prefix of string "s"
 */
bool prefix(std::string_view s, std::string_view t)
{
    //! @todo C++20 では starts_with が使用可能
    return s.substr(0, t.size()) == t;
}

/*
 * Redefinable "plog" action
 */
void (*plog_aux)(concptr) = nullptr;

/*
 * Print (or log) a "warning" message (ala "perror()")
 * Note the use of the (optional) "plog_aux" hook.
 */
void plog(concptr str)
{
    /* Use the "alternative" function if possible */
    if (plog_aux) {
        (*plog_aux)(str);
    }

    /* Just do a labeled fprintf to stderr */
    else {
        (void)(fprintf(stderr, "%s: %s\n", argv0 ? argv0 : "???", str));
    }
}

/*
 * Redefinable "quit" action
 */
void (*quit_aux)(concptr) = nullptr;

/*
 * Exit (ala "exit()").  If 'str' is nullptr, do "exit(0)".
 * If 'str' begins with "+" or "-", do "exit(atoi(str))".
 * Otherwise, plog() 'str' and exit with an error code of -1.
 * But always use 'quit_aux', if set, before anything else.
 */
void quit(concptr str)
{
    /* Attempt to use the aux function */
    if (quit_aux) {
        (*quit_aux)(str);
    }

    /* Success */
    if (!str) {
        (void)(exit(0));
    }

    /* Extract a "special error code" */
    if ((str[0] == '-') || (str[0] == '+')) {
        (void)(exit(atoi(str)));
    }

    /* Send the string to plog() */
    plog(str);

    /* Failure */
    (void)(exit(EXIT_FAILURE));
}

/*
 * Redefinable "core" action
 */
void (*core_aux)(concptr) = nullptr;

/*
 * Dump a core file, after printing a warning message
 * As with "quit()", try to use the "core_aux()" hook first.
 */
void core(concptr str)
{
    char *crash = nullptr;

    /* Use the aux function */
    if (core_aux) {
        (*core_aux)(str);
    }

    /* Dump the warning string */
    if (str) {
        plog(str);
    }

    /* Attempt to Crash */
    (*crash) = (*crash);

    /* Be sure we exited */
    quit("core() failed");
}

/*** 64-bit integer operations ***/

void s64b_lshift(int32_t *hi, uint32_t *lo, const int n)
{
    if (n == 0) {
        return;
    }

    *hi = (int32_t)((uint32_t)(*hi << n) | (*lo >> (32 - n)));
    *lo <<= n;
}

void s64b_rshift(int32_t *hi, uint32_t *lo, const int n)
{
    if (n == 0) {
        return;
    }

    *lo = ((uint32_t)*hi << (32 - n)) | (*lo >> n);
    *hi >>= n;
}

/* Add B to A */
void s64b_add(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2)
{
    (*A2) += B2;

    /* Overflawed? */
    if ((*A2) < B2) {
        (*A1) += B1 + 1;
    } else {
        (*A1) += B1;
    }
}

/* Subtract B from A */
void s64b_sub(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2)
{
    /* Underflaw? */
    if ((*A2) < B2) {
        (*A1) -= B1 + 1;
    } else {
        (*A1) -= B1;
    }

    (*A2) -= B2;
}

/*
 * Multiply A by B
 *
 * (A1*2^32 + A2h*2^16 + A2l) * (B1*2^32 + B2h*2^16 + B2l)
 *  = (A1*B2 & 0xffffffff)*2^32
 *   +(A2*B1 & 0xffffffff)*2^32
 *   +(A2h*B2h & 0xffffffff)*2^32
 *   +(A2h*B2l & 0xffff0000)*2^16
 *   +(A2l*B2h & 0xffff0000)*2^16
 *   +(A2*B2 & 0xffffffff)
 */
void s64b_mul(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2)
{
    int32_t tmp1;
    uint32_t A2val = (*A2);

    uint32_t B2high = (B2 >> 16);
    uint32_t A2high = (A2val >> 16);

    (*A2) *= B2;
    tmp1 = (*A1) * B2;
    tmp1 += A2val * B1;
    tmp1 += A2high * B2high;
    tmp1 += (A2high * (uint16_t)B2) >> 16;
    tmp1 += ((uint16_t)A2val * B2high) >> 16;

    (*A1) = tmp1;
}

/* Compare A to B */
int s64b_cmp(int32_t A1, uint32_t A2, int32_t B1, uint32_t B2)
{
    if (A1 > B1) {
        return 1;
    }
    if (A1 < B1) {
        return -1;
    }
    if (A2 > B2) {
        return 1;
    }
    if (A2 < B2) {
        return -1;
    }
    return 0;
}

/*
 * Divide A by B
 *
 * Assumes that both A and B are positive
 */
void s64b_div(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2)
{
    int32_t result1 = 0;
    uint32_t result2 = 0;
    int32_t A1val = (*A1);
    uint32_t A2val = (*A2);
    int bit = 0;

    /* No result for B==0 */
    if (B1 == 0 && B2 == 0) {
        return;
    }

    /*
     * Find the highest bit of quotient
     */
    while (s64b_cmp(A1val, A2val, B1, B2) == 1) {
        s64b_lshift(&B1, &B2, 1);
        bit++;
    }

    /* Extract bits of quotient one by one */
    while (bit >= 0) {
        if (s64b_cmp(A1val, A2val, B1, B2) >= 0) {
            if (bit >= 32) {
                result1 |= (0x00000001UL << (bit - 32));
            } else {
                result2 |= (0x00000001UL << bit);
            }

            s64b_sub(&A1val, &A2val, B1, B2);
        }

        s64b_rshift(&B1, &B2, 1);
        bit--;
    }

    (*A1) = result1;
    (*A2) = result2;
}

/* Reminder of ENERGY_DIVISION (A % B) */
void s64b_mod(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2)
{
    int32_t tmp1 = (*A1);
    uint32_t tmp2 = (*A2);

    s64b_div(&tmp1, &tmp2, B1, B2);
    s64b_mul(&tmp1, &tmp2, B1, B2);
    s64b_sub(A1, A2, tmp1, tmp2);
}

/*!
 * @brief 符号なし32ビット整数のビット数を返す。
 * @param x ビット数を調べたい変数
 * @return ビット数
 */
int count_bits(BIT_FLAGS x)
{
    int n = 0;

    if (x) {
        do {
            n++;
        } while (0 != (x = x & (x - 1)));
    }

    return n;
}
