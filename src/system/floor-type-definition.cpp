#include "system/floor-type-definition.h"
#include "system/dungeon-info.h"

bool FloorType::is_in_dungeon() const
{
    return this->dun_level > 0;
}

void FloorType::set_dungeon_index(short dungeon_idx_)
{
    this->dungeon_idx = dungeon_idx_;
}

void FloorType::reset_dungeon_index()
{
    this->set_dungeon_index(0);
}

dungeon_type &FloorType::get_dungeon_definition() const
{
    return dungeons_info[this->dungeon_idx];
}
