/*!
 * @brief オブジェクトの実装 / Object code, part 2
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "object/object2.h"

/*
 * todo この説明長すぎ。何とかしたい
 * Determine if an item can "absorb" a second item
 *
 * See "object_absorb()" for the actual "absorption" code.
 *
 * If permitted, we allow staffs (if they are known to have equal charges
 * and both are either known or confirmed empty) and wands (if both are
 * either known or confirmed empty) and rods (in all cases) to combine.
 * Staffs will unstack (if necessary) when they are used, but wands and
 * rods will only unstack if one is dropped. -LM-
 *
 * If permitted, we allow weapons/armor to stack, if fully "known".
 *
 * Missiles will combine if both stacks have the same "known" status.
 * This is done to make unidentified stacks of missiles useful.
 *
 * Food, potions, scrolls, and "easy know" items always stack.
 *
 * Chests, and activatable items, never stack (for various reasons).
 */

/*!
 * Function hook to restrict "get_obj_num_prep()" function
 */
bool(*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

OBJECT_SUBTYPE_VALUE coin_type;	/* Hack -- force coin type */
