#include "angband.h"
#include "dungeon.h"

/*
 * The dungeon arrays
 */
dungeon_type *d_info;
char *d_name;
char *d_text;

/*
 * Maximum number of dungeon in d_info.txt
 */
DUNGEON_IDX max_d_idx;
DEPTH *max_dlv;
