#pragma once

#include "spells.h"

TERM_COLOR mh_attr(int max);
TERM_COLOR spell_color(EFFECT_ID type);
u16b bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ);

extern TERM_COLOR gf_color[MAX_GF];
extern TERM_COLOR color_char_to_attr(SYMBOL_CODE c);
