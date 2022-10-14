#include "system/dungeon-info.h"
#include "dungeon/dungeon-flag-mask.h"

/*
 * The dungeon arrays
 */
std::vector<dungeon_type> dungeons_info;

/*
 * Maximum number of dungeon in DungeonDefinitions.txt
 */
std::vector<DEPTH> max_dlv;

bool dungeon_type::has_river_flag() const
{
    return this->flags.has_any_of(DF_RIVER_MASK);
}
