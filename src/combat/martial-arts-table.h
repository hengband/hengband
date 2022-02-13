#pragma once

#include "system/angband.h"

#define MAX_MA 17 /*!< 修行僧マーシャルアーツの技数 / Monk martial arts... */
#define MA_KNEE 1 /*!< 金的効果ID */
#define MA_SLOW 2 /*!< 膝蹴り効果ID */

/* For Monk martial arts */
struct martial_arts {
    concptr desc; /* A verbose attack description */
    PLAYER_LEVEL min_level; /* Minimum level to use */
    int chance; /* Chance of 'success' */
    int dd; /* Damage dice */
    int ds; /* Damage sides */
    int effect; /* Special effects */
};

extern const martial_arts ma_blows[MAX_MA];
