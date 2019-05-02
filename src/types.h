#pragma once

/*!
 * @file types.h
 * @brief グローバルな構造体の定義 / global type declarations
 * @date 2014/08/10
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 * @details
 * <pre>
 * このファイルはangband.hでのみインクルードすること。
 * This file should ONLY be included by "angband.h"
 *
 * Note that "char" may or may not be signed, and that "signed char"
 * may or may not work on all machines.  So always use "s16b" or "s32b"
 * for signed values.  Also, note that unsigned values cause math problems
 * in many cases, so try to only use "u16b" and "u32b" for "bit flags",
 * unless you really need the extra bit of information, or you really
 * need to restrict yourself to a single byte for storage reasons.
 *
 * Also, if possible, attempt to restrict yourself to sub-fields of
 * known size (use "s16b" or "s32b" instead of "int", and "byte" instead
 * of "bool"), and attempt to align all fields along four-byte words, to
 * optimize storage issues on 32-bit machines.  Also, avoid "bit flags"
 * since these increase the code size and slow down execution.  When
 * you need to store bit flags, use one byte per flag, or, where space
 * is an issue, use a "byte" or "u16b" or "u32b", and add special code
 * to access the various bit flags.
 *
 * Many of these structures were developed to reduce the number of global
 * variables, facilitate structured program design, allow the use of ascii
 * template files, simplify access to indexed data, or facilitate efficient
 * clearing of many variables at once.
 *
 * Certain data is saved in multiple places for efficient access, currently,
 * this includes the tval/sval/weight fields in "object_type", various fields
 * in "header_type", and the "m_idx" and "o_idx" fields in "grid_type".  All
 * of these could be removed, but this would, in general, slow down the game
 * and increase the complexity of the code.
 * </pre>
 */

#include "h-type.h"
#include "defines.h"
#include "object.h"

//#include "player-skill.h"


/*
 * The "name" of spell 'N' is stored as spell_names[X][N],
 * where X is 0 for mage-spells and 1 for priest-spells.
 */
typedef struct magic_type magic_type;

struct magic_type
{
	PLAYER_LEVEL slevel;	/* Required level (to learn) */
	MANA_POINT smana;		/* Required mana (to cast) */
	PERCENTAGE sfail;		/* Minimum chance of failure */
	EXP sexp;				/* Encoded experience bonus */
};

typedef bool (*monsterrace_hook_type)(MONRACE_IDX r_idx);

/*
 * This seems like a pretty standard "typedef"
 */
typedef int (*inven_func)(object_type *);
