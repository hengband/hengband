#include "angband.h"
#include "util.h"
#include "objectkind.h"

/*
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/*
 * The size of "alloc_kind_table" (at most max_k_idx * 4)
 */
s16b alloc_kind_size;

/*
 * The entries in the "kind allocator table"
 */
alloc_entry *alloc_kind_table;

