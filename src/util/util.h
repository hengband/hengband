#pragma once

#include "floor/geometry.h"

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 * Pass 3 is determined from allocation calculation
 */

typedef struct alloc_entry alloc_entry;

struct alloc_entry {
    KIND_OBJECT_IDX index; /* The actual index */

    DEPTH level; /* Base dungeon level */
    PROB prob1; /* Probability, pass 1 */
    PROB prob2; /* Probability, pass 2 */
    PROB prob3; /* Probability, pass 3 */

    u16b total; /* Unused for now */
};
