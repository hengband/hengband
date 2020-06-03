#pragma once

#include "system/angband.h"

extern int init_flags;
extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);
