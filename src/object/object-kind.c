#include "object-kind.h"
#include "util/util.h"

/*
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/*
 * Maximum number of items in k_info.txt
 */
KIND_OBJECT_IDX max_k_idx;

/*
 * The size of "alloc_kind_table" (at most max_k_idx * 4)
 */
s16b alloc_kind_size;

/*
 * The entries in the "kind allocator table"
 */
alloc_entry *alloc_kind_table;

