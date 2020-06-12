#pragma once

#include "system/angband.h"
#include "monster-race/monster-race.h"

typedef enum monster_sex {
    MSEX_NONE = 0,
    MSEX_MALE = 1,
    MSEX_FEMALE = 2,
} monster_sex;

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
    bool know_everything;
    BIT_FLAGS mode;
    monster_sex msex;
    bool old;
    MONRACE_IDX r_idx;
    int vn;
    byte color[96];
    concptr vp[96];
    char tmp_msg[96][96];
    bool breath;
} lore_type;

typedef void (*hook_c_roff_pf)(TERM_COLOR attr, concptr str);
extern hook_c_roff_pf hook_c_roff;

extern concptr wd_he[3];
extern concptr wd_his[3];

lore_type *initialize_lore_type(lore_type *lore_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
void hooked_roff(concptr str);
