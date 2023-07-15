#pragma once

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include "system/h-basic.h"
#include <string_view>

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
bool streq(std::string_view s, std::string_view t);
bool prefix(std::string_view s, std::string_view t);
bool suffix(std::string_view s, std::string_view t);

/* Print an error message */
void plog(concptr str);

/* Exit, with optional message */
void quit(concptr str);

/* Dump core, with optional message */
void core(concptr str);

/* 64-bit integer operations */

/*!
 * @brief 64bit非負整数を n 回左シフトする。
 * @param hi 上位32bit。負であってはならない。
 * @param lo 下位32bit。
 * @param n  シフト量。[0,31] の範囲でなければならない。
 *
 * hi や n に範囲外の値を渡したり、オーバーフローした場合の動作は未定義。
 */
void s64b_lshift(int32_t *hi, uint32_t *lo, int n);

/*!
 * @brief 64bit非負整数を n 回右シフトする。
 * @param hi 上位32bit。負であってはならない。
 * @param lo 下位32bit。
 * @param n シフト量。[0,31] の範囲でなければならない。
 *
 * hi や n に範囲外の値を渡した場合の動作は未定義。
 */
void s64b_rshift(int32_t *hi, uint32_t *lo, int n);

void s64b_add(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2);
void s64b_sub(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2);
int s64b_cmp(int32_t A1, uint32_t A2, int32_t B1, uint32_t B2);
void s64b_mul(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2);
void s64b_div(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2);
void s64b_mod(int32_t *A1, uint32_t *A2, int32_t B1, uint32_t B2);

int count_bits(BIT_FLAGS x);
