#include "info-reader/general-parser.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include <stdexcept>

dungeon_grid letter[255];

dungeon_grid &fixed_map_letter_at(unsigned char symbol)
{
    if (!is_fixed_map_symbol(symbol)) {
        throw std::out_of_range("Invalid fixed map symbol");
    }

    return letter[symbol];
}

void dungeon_grid::set_terrain_id(TerrainTag tag)
{
    this->feature = TerrainList::get_instance().get_terrain_id(tag);
}

void dungeon_grid::set_trap_id(TerrainTag tag)
{
    this->trap = TerrainList::get_instance().get_terrain_id(tag);
}
