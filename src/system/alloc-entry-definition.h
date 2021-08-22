/*
 * @brief
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2002/01/12 mogami
 * 2020/05/16 Hourier
 * @details
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#pragma once

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 */
typedef struct alloc_entry {
    KIND_OBJECT_IDX index; /* The actual index */

    DEPTH level; /* Base dungeon level */
    PROB prob1; /* Probability, pass 1 */
    PROB prob2; /* Probability, pass 2 */

    uint16_t total; /* Unused for now */
} alloc_entry;
