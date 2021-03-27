#pragma once

#include "system/angband.h"

/*
 * Size of memory reserved for initialization of some arrays
 */
extern int error_idx;
extern int error_line;

typedef struct angband_header angband_header;
errr grab_one_flag(u32b *flags, concptr names[], concptr what);
byte grab_one_activation_flag(concptr what);
