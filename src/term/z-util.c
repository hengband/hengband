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
concptr argv0 = NULL;


/*
 * Determine if string "t" is equal to string "t"
 */
bool streq(concptr a, concptr b)
{
	return (!strcmp(a, b));
}


/*
 * Determine if string "t" is a suffix of string "s"
 */
bool suffix(concptr s, concptr t)
{
	int tlen = strlen(t);
	int slen = strlen(s);

	/* Check for incompatible lengths */
	if (tlen > slen) return FALSE;

	/* Compare "t" to the end of "s" */
	return (!strcmp(s + slen - tlen, t));
}


/*
 * Determine if string "t" is a prefix of string "s"
 */
bool prefix(concptr s, concptr t)
{
	/* Scan "t" */
	while (*t)
	{
		/* Compare content and length */
		if (*t++ != *s++) return FALSE;
	}

	/* Matched, we have a prefix */
	return TRUE;
}



/*
 * Redefinable "plog" action
 */
void (*plog_aux)(concptr) = NULL;

/*
 * Print (or log) a "warning" message (ala "perror()")
 * Note the use of the (optional) "plog_aux" hook.
 */
void plog(concptr str)
{
	/* Use the "alternative" function if possible */
	if (plog_aux) (*plog_aux)(str);

	/* Just do a labeled fprintf to stderr */
	else (void)(fprintf(stderr, "%s: %s\n", argv0 ? argv0 : "???", str));
}



/*
 * Redefinable "quit" action
 */
void (*quit_aux)(concptr) = NULL;

/*
 * Exit (ala "exit()").  If 'str' is NULL, do "exit(0)".
 * If 'str' begins with "+" or "-", do "exit(atoi(str))".
 * Otherwise, plog() 'str' and exit with an error code of -1.
 * But always use 'quit_aux', if set, before anything else.
 */
void quit(concptr str)
{
	/* Attempt to use the aux function */
	if (quit_aux) (*quit_aux)(str);

	/* Success */
	if (!str) (void)(exit(0));

	/* Extract a "special error code" */
	if ((str[0] == '-') || (str[0] == '+')) (void)(exit(atoi(str)));

	/* Send the string to plog() */
	plog(str);

	/* Failure */
	(void)(exit(EXIT_FAILURE));
}



/*
 * Redefinable "core" action
 */
void (*core_aux)(concptr) = NULL;

/*
 * Dump a core file, after printing a warning message
 * As with "quit()", try to use the "core_aux()" hook first.
 */
void core(concptr str)
{
	char *crash = NULL;

	/* Use the aux function */
	if (core_aux) (*core_aux)(str);

	/* Dump the warning string */
	if (str) plog(str);

	/* Attempt to Crash */
	(*crash) = (*crash);

	/* Be sure we exited */
	quit("core() failed");
}


/*** 64-bit integer operations ***/

/* Add B to A */
void s64b_add(s32b *A1, u32b *A2, s32b B1, u32b B2)
{
	(*A2) += B2;

	/* Overflawed? */
	if ((*A2) < B2)
		(*A1) += B1 + 1;
	else
		(*A1) += B1;
}


/* Subtract B from A */
void s64b_sub(s32b *A1, u32b *A2, s32b B1, u32b B2)
{
	/* Underflaw? */
	if ((*A2) < B2)
		(*A1) -= B1 + 1;
	else
		(*A1) -= B1;

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
void s64b_mul(s32b *A1, u32b *A2, s32b B1, u32b B2)
{
	s32b tmp1;
	u32b A2val = (*A2);

	u32b B2high = (B2 >> 16);
	u32b A2high = (A2val >> 16);

	(*A2) *= B2;
	tmp1 = (*A1) * B2;
	tmp1 += A2val * B1;
	tmp1 += A2high * B2high;
	tmp1 += (A2high * (u16b)B2) >> 16;
	tmp1 += ((u16b)A2val * B2high) >> 16;

	(*A1) = tmp1;
}


/* Compare A to B */
int s64b_cmp(s32b A1, u32b A2, s32b B1, u32b B2)
{
	if (A1 > B1) return 1;
	if (A1 < B1) return -1;
	if (A2 > B2) return 1;
	if (A2 < B2) return -1;
	return 0;
}

/*
 * Divide A by B
 *
 * Assumes that both A and B are positive
 */
void s64b_div(s32b *A1, u32b *A2, s32b B1, u32b B2)
{
	s32b result1 = 0;
	u32b result2 = 0;
	s32b A1val = (*A1);
	u32b A2val = (*A2);
	int bit = 0;

	/* No result for B==0 */
	if (B1 == 0 && B2 == 0) return;

	/*
	 * Find the highest bit of quotient
	 */
	while (s64b_cmp(A1val, A2val, B1, B2) == 1)
	{
		s64b_LSHIFT(B1, B2, 1);
		bit++;
	}

	/* Extract bits of quotient one by one */
	while (bit >= 0)
	{
		if (s64b_cmp(A1val, A2val, B1, B2) >= 0)
		{
			if (bit >= 32)
				result1 |= (0x00000001L << (bit - 32));
			else
				result2 |= (0x00000001L << bit);

			s64b_sub(&A1val, &A2val, B1, B2);
		}
	
		s64b_RSHIFT(B1, B2, 1);
		bit--;
	}

	(*A1) = result1;
	(*A2) = result2;
}


/* Reminder of division (A % B) */
void s64b_mod(s32b *A1, u32b *A2, s32b B1, u32b B2)
{
	s32b tmp1 = (*A1);
	u32b tmp2 = (*A2);

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

	if (x) do
	{
		n++;
	} while (0 != (x = x&(x - 1)));

	return (n);
}

/*!
 * @brief 平方根を切り捨て整数で返す
 * @param n 数値
 * @return 平方根
 */
int mysqrt(int n)
{
	int tmp = n >> 1;
	int tasu = 10;
	int kaeriti = 1;

	if (!tmp)
	{
		if (n) return 1;
		else return 0;
	}

	while (tmp)
	{
		if ((n / tmp) < tmp)
		{
			tmp >>= 1;
		}
		else break;
	}
	kaeriti = tmp;
	while (tasu)
	{
		if ((n / tmp) < tmp)
		{
			tasu--;
			tmp = kaeriti;
		}
		else
		{
			kaeriti = tmp;
			tmp += tasu;
		}
	}
	return kaeriti;
}