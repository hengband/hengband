#pragma once

#include "player/special-defense-types.h"
#include "system/angband.h"

typedef struct blow_stance {
    concptr desc; /* A verbose stance description */
    PLAYER_LEVEL min_level; /* Minimum level to use */
    concptr info;
} blow_stance;

extern const blow_stance monk_stances[MAX_KAMAE];
extern const blow_stance samurai_stances[MAX_KATA];
