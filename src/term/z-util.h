﻿/* File z-util.h */

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

/*!
 * @brief 64bit非負整数を n 回左シフトする。
 * @param hi 上位32bit。負であってはならない。
 * @param lo 下位32bit。
 * @param n  シフト量。[0,31] の範囲でなければならない。
 *
 * hi や n に範囲外の値を渡したり、オーバーフローした場合の動作は未定義。
 */
void s64b_lshift(s32b* hi, u32b* lo, int n);

/*!
 * @brief 64bit非負整数を n 回右シフトする。
 * @param hi 上位32bit。負であってはならない。
 * @param lo 下位32bit。
 * @param n シフト量。[0,31] の範囲でなければならない。
 *
 * hi や n に範囲外の値を渡した場合の動作は未定義。
 */
void s64b_rshift(s32b* hi, u32b* lo, int n);

extern void s64b_add(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_sub(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern int s64b_cmp(s32b A1, u32b A2, s32b B1, u32b B2);
extern void s64b_mul(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_div(s32b *A1, u32b *A2, s32b B1, u32b B2);
extern void s64b_mod(s32b *A1, u32b *A2, s32b B1, u32b B2);


#endif

extern int count_bits(BIT_FLAGS x);
extern int mysqrt(int n);

