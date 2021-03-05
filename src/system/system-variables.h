﻿#pragma once

#include "system/angband.h"

#define MAX_NAZGUL_NUM 5
#define SCREEN_BUF_MAX_SIZE (1024 * 1024) /*!< Max size of screen dump buffer */

enum init_flags_type {
	INIT_NAME_ONLY = 0x01,
    INIT_SHOW_TEXT = 0x02,
    INIT_ASSIGN = 0x04,
    INIT_CREATE_DUNGEON = 0x08,
    INIT_ONLY_FEATURES = 0x10,
    INIT_ONLY_BUILDINGS = 0x20,
};

extern init_flags_type init_flags;
extern concptr ANGBAND_SYS;
extern concptr ANGBAND_KEYBOARD;
extern concptr ANGBAND_GRAF;

extern OBJECT_SUBTYPE_VALUE coin_type;
extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);
