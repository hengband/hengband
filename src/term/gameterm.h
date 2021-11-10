#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

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

extern term_type *angband_term[8];
#define term_screen (angband_term[0])

extern TERM_COLOR misc_to_attr[256];
extern SYMBOL_CODE misc_to_char[256];
extern TERM_COLOR tval_to_attr[128];
extern const char angband_term_name[8][16];
extern byte angband_color_table[256][4];

extern TERM_COLOR gf_color[(int)AttributeType::MAX];
extern TERM_COLOR color_char_to_attr(SYMBOL_CODE c);

uint16_t bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ);
