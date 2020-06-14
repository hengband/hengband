#include "system/angband.h"
#include "monster-race/monster-race.h"
#include "util/util.h"

monster_race *r_info;
char *r_name;
char *r_text;

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
