﻿#pragma once

#include "player/player-class-types.h"
#include "player/player-personality-types.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "system/angband.h"
#include "system/system-variables.h"

/*
 * A structure to hold "rolled" information
 */
struct birther {
    player_sex psex{}; /* Sex index */
    player_race_type prace{}; /* Race index */
    player_class_type pclass{}; /* Class index */
    player_personality_type pseikaku{}; /* Seikaku index */
    REALM_IDX realm1{}; /* First magic realm */
    REALM_IDX realm2{}; /* Second magic realm */

    short age{};
    short ht{};
    short wt{};
    short sc{};

    PRICE au{}; /*!< 初期の所持金 */

    BASE_STATUS stat_max[6]{}; /* Current "maximal" stat values */
    BASE_STATUS stat_max_max[6]{}; /* Maximal "maximal" stat values */
    HIT_POINT player_hp[PY_MAX_LEVEL]{};

    PATRON_IDX chaos_patron{};

    short vir_types[8]{};

    char history[4][60]{};

    bool quick_ok{};
};

extern birther previous_char;

struct player_type;
bool ask_quick_start(player_type *creature_ptr);
void save_prev_data(player_type *creature_ptr, birther *birther_ptr);
void load_prev_data(player_type *creature_ptr, bool swap);
