#include "room/door-definition.h"

Door::Door()
{
    constexpr auto max_lock_jammed_doors = 8;
    this->locked.resize(max_lock_jammed_doors);
    this->jammed.resize(max_lock_jammed_doors);
}

std::map<DoorKind, Door> feat_door;
