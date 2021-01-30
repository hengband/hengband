#pragma once

#include "system/angband.h"

/*
 * Available "options"
 *	- Address of actual option variable (or NULL)
 *	- Normal Value (TRUE or FALSE)
 *	- Option Page Number (or zero)
 *	- Savefile Set (or zero)
 *	- Savefile Bit in that set
 *	- Textual name (or NULL)
 *	- Textual description
 */
typedef struct option_type {
    bool *o_var;
    byte o_norm;
    byte o_page;
    byte o_set;
    byte o_bit;
    concptr o_text;
    concptr o_desc;
} option_type;

#define MAX_OPTION_INFO 121
#define MAX_CHEAT_OPTIONS 10
#define MAX_AUTOSAVE_INFO 2

extern const option_type option_info[MAX_OPTION_INFO];
extern const option_type cheat_info[MAX_CHEAT_OPTIONS];
extern const option_type autosave_info[MAX_AUTOSAVE_INFO];
