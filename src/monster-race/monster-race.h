#pragma once

#include "system/monster-race-definition.h"
#include "util/util.h"

/*
 * The monster race arrays
 */
extern monster_race *r_info;
extern char *r_name;
extern char *r_text;

extern s16b alloc_race_size;
extern alloc_entry *alloc_race_table;
extern MONRACE_IDX max_r_idx;
