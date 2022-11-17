/*
 * @brief
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2002/01/12 mogami
 * 2021/10/02 Hourier
 */

#pragma once

#include "system/angband.h"
#include <vector>

/*
 * An entry for the object/monster allocation functions
 *
 * Pass 1 is determined from allocation information
 * Pass 2 is determined from allocation restriction
 */
struct alloc_entry {
    short index; /* The actual index */

    DEPTH level; /* Base dungeon level */
    PROB prob1; /* Probability, pass 1 */
    PROB prob2; /* Probability, pass 2 */

    uint16_t total; /* Unused for now */
};

extern std::vector<alloc_entry> alloc_race_table;

extern std::vector<alloc_entry> alloc_kind_table;
