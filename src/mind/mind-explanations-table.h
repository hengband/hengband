#pragma once

#include "system/angband.h"

#define MAX_MINDKINDS 5 /* 職業特有の特殊能力数 */
#define MAX_MIND_POWERS 21 /*!< 超能力の数 / Mindcraft */

struct mind_type {
    PLAYER_LEVEL min_lev;
    MANA_POINT mana_cost;
    PERCENTAGE fail;
    concptr name;
};

struct mind_power {
    mind_type info[MAX_MIND_POWERS];
};

extern mind_power const mind_powers[MAX_MINDKINDS];
extern concptr const mind_tips[MAX_MINDKINDS][MAX_MIND_POWERS];
