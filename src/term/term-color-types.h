#pragma once

/*
 * Angband "attributes" (with symbols, and base (R,G,B) codes)
 *
 * The "(R,G,B)" codes are given in "fourths" of the "maximal" value,
 * and should "gamma corrected" on most (non-Macintosh) machines.
 */
enum term_color_type : unsigned char {
	TERM_DARK = 0, /*!< 'd' - 黒 0,0,0 */
    TERM_WHITE = 1, /*!< 'w' - 白 4,4,4 */
    TERM_SLATE = 2, /*!< 's' - 灰 2,2,2 */
    TERM_ORANGE = 3, /*!< 'o' - 橙 4,2,0 */
    TERM_RED = 4, /*!< 'r' - 赤 3,0,0 */
    TERM_GREEN = 5, /*!< 'g' - 緑 0,2,1 */
    TERM_BLUE = 6, /*!< 'b' - 青 0,0,4 */
    TERM_UMBER = 7, /*!< 'u' - 琥珀 2,1,0 */
    TERM_L_DARK = 8, /*!< 'D' - 暗い灰 1,1,1 */
    TERM_L_WHITE = 9, /*!< 'W' - 明るい灰 3,3,3 */
    TERM_VIOLET = 10, /*!< 'v' - 紫 4,0,4 */
    TERM_YELLOW = 11, /*!< 'y' - 黄 4,4,0 */
    TERM_L_RED = 12, /*!< 'R' - 明るい赤 4,0,0 */
    TERM_L_GREEN = 13, /*!< 'G' - 明るい緑 0,4,0 */
    TERM_L_BLUE = 14, /*!< 'B' - 明るい青 0,4,4 */
    TERM_L_UMBER = 15, /*!< 'U' - 明るい琥珀 3,2,1 */
};
