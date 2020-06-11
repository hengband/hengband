#pragma once

#include "system/angband.h"
#include "monster-race/monster-race.h"

typedef struct lore_type {
#ifdef JP
    char jverb_buf[64];
#else
    bool sin = FALSE;
#endif
    bool nightmare;
    monster_race *r_ptr;
    SPEED speed;
    ITEM_NUMBER drop_gold;
    ITEM_NUMBER drop_item;
    BIT_FLAGS flags1;
    BIT_FLAGS flags2;
    BIT_FLAGS flags3;
    BIT_FLAGS flags4;
    BIT_FLAGS a_ability_flags1;
    BIT_FLAGS a_ability_flags2;
    BIT_FLAGS flags7;
    BIT_FLAGS flagsr;
    bool reinforce;
} lore_type;

lore_type *initialize_lore_type(lore_type *lore_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
