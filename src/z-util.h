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

#include "h-basic.h"


/*
 * Extremely basic stuff, like global temp and constant variables.
 * Also, some very useful low level functions, such as "streq()".
 * All variables and functions in this file are "addressable".
 */


/**** Available variables ****/

/* A cptr to the name of the program */
extern cptr argv0;


/* Aux functions */
extern void (*plog_aux)(cptr);
extern void (*quit_aux)(cptr);
extern void (*core_aux)(cptr);


/**** Available Functions ****/

/* Test equality, prefix, suffix */
extern bool streq(cptr s, cptr t);
extern bool prefix(cptr s, cptr t);
extern bool suffix(cptr s, cptr t);


/* Print an error message */
extern void plog(cptr str);

/* Exit, with optional message */
extern void quit(cptr str);

/* Dump core, with optional message */
extern void core(cptr str);


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

