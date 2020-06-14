#pragma once

#include "floor/geometry.h"

#define SCREEN_BUF_MAX_SIZE (4 * 65536) /*!< Max size of screen dump buffer */

/*
 * Max numbers of macro trigger names
 */
#define MAX_MACRO_MOD 12
#define MAX_MACRO_TRIG 200 /*!< 登録を許すマクロ（トリガー）の最大数 */

/*
 *Language selection macro
 */
#ifdef JP
#define _(JAPANESE,ENGLISH) (JAPANESE)
#else
#define _(JAPANESE,ENGLISH) (ENGLISH)
#endif

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */

typedef struct alloc_entry alloc_entry;

struct alloc_entry
{
	KIND_OBJECT_IDX index;		/* The actual index */

	DEPTH level;		/* Base dungeon level */
	PROB prob1;		/* Probability, pass 1 */
	PROB prob2;		/* Probability, pass 2 */
	PROB prob3;		/* Probability, pass 3 */

	u16b total;		/* Unused for now */
};

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX  81920

/*
 * OPTION: Maximum space for the message text buffer (see "io.c")
 * Default: assume that each of the 2048 messages is repeated an
 * average of three times, and has an average length of 48
 */
#define MESSAGE_BUF 655360

/*
 * Hack -- The main "screen"
 */
#define term_screen     (angband_term[0])

/*
 * Hack -- conditional (or "bizarre") externs
 */

#ifdef SET_UID
extern void user_name(char *buf, int id);
#endif

#ifndef HAVE_USLEEP
extern int usleep(huge usecs);
#endif

extern errr type_string(concptr str, uint len);
extern void roff_to_buf(concptr str, int wlen, char *tbuf, size_t bufsize);

extern size_t angband_strcpy(char *buf, concptr src, size_t bufsize);
extern size_t angband_strcat(char *buf, concptr src, size_t bufsize);
extern char *angband_strstr(concptr haystack, concptr needle);
extern char *angband_strchr(concptr ptr, char ch);
extern void str_tolower(char *str);
