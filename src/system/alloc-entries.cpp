#include "system/alloc-entries.h"
#include "system/baseitem-info.h"

/* The entries in the "race allocator table" */
std::vector<alloc_entry> alloc_race_table;

/* The entries in the "kind allocator table" */
std::vector<alloc_entry> alloc_kind_table;

BaseitemInfo &alloc_entry::get_baseitem() const
{
    return baseitems_info[this->index];
}
