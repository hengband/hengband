#pragma once

#include "system/angband.h"

extern const char hexsym[16];

/*
 * Automatically generated "variable" declarations
 */
extern int max_macrotrigger;
extern concptr macro_template;
extern concptr macro_modifier_chr;
extern concptr macro_modifier_name[MAX_MACRO_MOD];
extern concptr macro_trigger_name[MAX_MACRO_TRIG];
extern concptr macro_trigger_keycode[2][MAX_MACRO_TRIG];

void text_to_ascii(char *buf, concptr str);
void ascii_to_text(char *buf, concptr str);
