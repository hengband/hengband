#include "system/alloc-entries.h"

/* The size of "alloc_race_table" (at most max_r_idx) */
int16_t alloc_race_size;

/* The entries in the "race allocator table" */
alloc_entry *alloc_race_table;

/* The size of "alloc_kind_table" (at most max_k_idx * 4) */
int16_t alloc_kind_size;

/* The entries in the "kind allocator table" */
alloc_entry *alloc_kind_table;
