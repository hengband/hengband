/* File: z-rand.h */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#ifndef INCLUDED_Z_RAND_H
#define INCLUDED_Z_RAND_H

#include "system/h-basic.h"

/**** Available constants ****/

/*
 * Random Number Generator -- Degree of "complex" RNG -- see "misc.c"
 * This value is hard-coded at 63 for a wide variety of reasons.
 */
#define RAND_DEG 63

/**** Available macros ****/

/*
 * Generates a random long integer X where O<=X<M.
 * The integer X falls along a uniform distribution.
 * For example, if M is 100, you get "percentile dice"
 */
#define randint0(M) ((int32_t)Rand_div(M))

/*
 * Generates a random long integer X where A<=X<=B
 * The integer X falls along a uniform distribution.
 * Note: rand_range(0,N-1) == randint0(N)
 */
#define rand_range(A, B) ((A) + (randint0(1 + (B) - (A))))

/*
 * Generate a random long integer X where A-D<=X<=A+D
 * The integer X falls along a uniform distribution.
 * Note: rand_spread(A,D) == rand_range(A-D,A+D)
 */
#define rand_spread(A, D) ((A) + (randint0(1 + (D) + (D))) - (D))

/*
 * Generate a random long integer X where 1<=X<=M
 * Also, "correctly" handle the case of M<=1
 */
#define randint1(M) (randint0(M) + 1)

/*
 * Evaluate to TRUE "P" percent of the time
 */
#define magik(P) (randint0(100) < (P))

#define one_in_(X) (randint0(X) == 0)

/*
 * Evaluate to TRUE "S" percent of the time
 */
#define saving_throw(S) (randint0(100) < (S))

extern uint16_t Rand_place;
extern uint32_t Rand_state[RAND_DEG];

void Rand_state_init(void);
void Rand_state_set(uint32_t seed);
void Rand_state_backup(uint32_t *backup_state);
void Rand_state_restore(uint32_t *backup_state);
int32_t Rand_div(int32_t m);
int16_t randnor(int mean, int stand);
int16_t damroll(DICE_NUMBER num, DICE_SID sides);
int16_t maxroll(DICE_NUMBER num, DICE_SID sides);
int32_t div_round(int32_t n, int32_t d);
int32_t Rand_external(int32_t m);
bool next_bool();

#endif
