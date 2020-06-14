#pragma once

#include "system/angband.h"

extern concptr *macro__pat;
extern concptr *macro__act;
extern s16b macro__num;

sint macro_find_exact(concptr pat);
sint macro_find_check(concptr pat);
errr macro_add(concptr pat, concptr act);
sint macro_find_maybe(concptr pat);
sint macro_find_ready(concptr pat);
