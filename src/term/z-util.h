/* File z-util.h */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#ifndef INCLUDED_Z_UTIL_H
#define INCLUDED_Z_UTIL_H

#include "system/h-basic.h"


/*
 * Extremely basic stuff, like global temp and constant variables.
 * Also, some very useful low level functions, such as "streq()".
 * All variables and functions in this file are "addressable".
 */


/**** Available variables ****/

/* A concptr to the name of the program */
extern concptr argv0;


/* Aux functions */
extern void (*plog_aux)(concptr);
extern void (*quit_aux)(concptr);
extern void (*core_aux)(concptr);


/**** Available Functions ****/

/* Test equality, prefix, suffix */
extern bool streq(concptr s, concptr t);
extern bool prefix(concptr s, concptr t);
extern bool suffix(concptr s, concptr t);


/* Print an error message */
extern void plog(concptr str);

/* Exit, with optional message */
extern void quit(concptr str);

/* Dump core, with optional message */
extern void core(concptr str);


/* 64-bit integer operations */
#define s64b_LSHIFT(V1, V2, N) {V1 = (V1<<(N)) | (V2>>(32-(N))); V2 <<= (N);}
#define s64b_RSHIFT(V1, V2, N) {V2 = (V1<<(32-(N))) | (V2>>(N)); V1 >>= (N);}
extern void s64b_add(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_sub(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern int s64b_cmp(s32b A1, u32b A2, s32b B1, u32b B2);
extern void s64b_mul(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_div(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_mod(s32b *A1, u32b *A2, s32b B1, u32b B2);


#endif

extern int count_bits(BIT_FLAGS x);
extern int mysqrt(int n);

