#include "system/angband.h"
#include "monster/monster-race.h"
#include "util.h"

extern monster_race *r_info;
extern char *r_name;
extern char *r_text;

/*
 * The size of "alloc_race_table" (at most max_r_idx)
 */
s16b alloc_race_size;

/*
 * The entries in the "race allocator table"
 */
alloc_entry *alloc_race_table;

/*
 * Maximum number of monsters in r_info.txt
 */
MONRACE_IDX max_r_idx;
