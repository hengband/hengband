#pragma once

#include "spell/spells-type.h"
/*
 * Angband "attributes" (with symbols, and base (R,G,B) codes)
 *
 * The "(R,G,B)" codes are given in "fourths" of the "maximal" value,
 * and should "gamma corrected" on most (non-Macintosh) machines.
 */
#define TERM_DARK                0  /*!< 'd' - 黒 0,0,0 */
#define TERM_WHITE               1  /*!< 'w' - 白 4,4,4 */
#define TERM_SLATE               2  /*!< 's' - 灰 2,2,2 */
#define TERM_ORANGE              3  /*!< 'o' - 橙 4,2,0 */
#define TERM_RED                 4  /*!< 'r' - 赤 3,0,0 */
#define TERM_GREEN               5  /*!< 'g' - 緑 0,2,1 */
#define TERM_BLUE                6  /*!< 'b' - 青 0,0,4 */
#define TERM_UMBER               7  /*!< 'u' - 琥珀 2,1,0 */
#define TERM_L_DARK              8  /*!< 'D' - 暗い灰 1,1,1 */
#define TERM_L_WHITE             9  /*!< 'W' - 明るい灰 3,3,3 */
#define TERM_VIOLET             10  /*!< 'v' - 紫 4,0,4 */
#define TERM_YELLOW             11  /*!< 'y' - 黄 4,4,0 */
#define TERM_L_RED              12  /*!< 'R' - 明るい赤 4,0,0 */
#define TERM_L_GREEN            13  /*!< 'G' - 明るい緑 0,4,0 */
#define TERM_L_BLUE             14  /*!< 'B' - 明るい青 0,4,4 */
#define TERM_L_UMBER            15  /*!< 'U' - 明るい琥珀 3,2,1 */

 /*** Macro Definitions ***/
 /*
  * Convert an "attr"/"char" pair into a "pict" (P)
  */
#define PICT(A,C) \
	((((u16b)(A)) << 8) | ((byte)(C)))

  /*
   * Convert a "pict" (P) into an "attr" (A)
   */
#define PICT_A(P) \
	((byte)((P) >> 8))

   /*
	* Convert a "pict" (P) into an "char" (C)
	*/
#define PICT_C(P) \
	((char)((byte)(P)))

extern const concptr color_names[16];
extern const concptr window_flag_desc[32];
extern const concptr ident_info[];
extern const concptr color_char;

extern term *angband_term[8];

extern TERM_COLOR misc_to_attr[256];
extern SYMBOL_CODE misc_to_char[256];
extern TERM_COLOR tval_to_attr[128];
extern SYMBOL_CODE tval_to_char[128];

extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];

TERM_COLOR mh_attr(int max);
TERM_COLOR spell_color(EFFECT_ID type);
u16b bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ);

extern TERM_COLOR gf_color[MAX_GF];
extern TERM_COLOR color_char_to_attr(SYMBOL_CODE c);
