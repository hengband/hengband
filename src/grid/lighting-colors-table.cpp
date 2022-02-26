#include "grid/lighting-colors-table.h"
#include "term/term-color-types.h"

/*!
 * 照明の表現を行うための色合いの関係を{暗闇時, 照明時} で定義する /
 * This array lists the effects of "brightness" on various "base" colours.\n
 *\n
 * This is used to do dynamic lighting effects in ascii :-)\n
 * At the moment, only the various "floor" tiles are affected.\n
 *\n
 * The layout of the array is [x][0] = light and [x][1] = dark.\n
 */
TERM_COLOR lighting_colours[MAX_COLORS][MAX_TIME_ZONES] = {
    /* TERM_DARK */
    { TERM_L_DARK, TERM_DARK },

    /* TERM_WHITE */
    { TERM_YELLOW, TERM_SLATE },

    /* TERM_SLATE */
    { TERM_WHITE, TERM_L_DARK },

    /* TERM_ORANGE */
    { TERM_L_UMBER, TERM_UMBER },

    /* TERM_RED */
    { TERM_RED, TERM_RED },

    /* TERM_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_BLUE */
    { TERM_BLUE, TERM_BLUE },

    /* TERM_UMBER */
    { TERM_L_UMBER, TERM_RED },

    /* TERM_L_DARK */
    { TERM_SLATE, TERM_L_DARK },

    /* TERM_L_WHITE */
    { TERM_WHITE, TERM_SLATE },

    /* TERM_VIOLET */
    { TERM_L_RED, TERM_BLUE },

    /* TERM_YELLOW */
    { TERM_YELLOW, TERM_ORANGE },

    /* TERM_L_RED */
    { TERM_L_RED, TERM_L_RED },

    /* TERM_L_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_L_BLUE */
    { TERM_L_BLUE, TERM_L_BLUE },

    /* TERM_L_UMBER */
    { TERM_L_UMBER, TERM_UMBER }
};
