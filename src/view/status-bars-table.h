#pragma once

#include "system/angband.h"

typedef struct stat_bar {
    TERM_COLOR attr;
    concptr sstr;
    concptr lstr;
} stat_bar;

#define MAX_STAT_BARS 68

extern stat_bar stat_bars[MAX_STAT_BARS];
