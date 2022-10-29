#include "system/floor-type-definition.h"

bool FloorType::is_in_dungeon() const
{
    return this->dun_level > 0;
}
