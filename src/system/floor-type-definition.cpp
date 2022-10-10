#include "system/floor-type-definition.h"

bool floor_type::is_in_dungeon() const
{
    return this->dun_level > 0;
}
