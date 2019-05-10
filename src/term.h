#pragma once

#include "spells.h"

extern const concptr color_names[16];
extern const concptr window_flag_desc[32];
extern const concptr ident_info[];
extern const concptr color_char;

extern TERM_COLOR misc_to_attr[256];
extern SYMBOL_CODE misc_to_char[256];
extern TERM_COLOR tval_to_attr[128];
extern SYMBOL_CODE tval_to_char[128];

TERM_COLOR mh_attr(int max);
TERM_COLOR spell_color(EFFECT_ID type);
u16b bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ);

extern TERM_COLOR gf_color[MAX_GF];
extern TERM_COLOR color_char_to_attr(SYMBOL_CODE c);
