#include "room/door-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "term/z-rand.h"
#include "util/enum-range.h"

Doors Doors::instance{};

Doors::Doors()
{
    constexpr EnumRangeInclusive<TerrainTag> locked_door_tags(TerrainTag::LOCKED_DOOR_1, TerrainTag::LOCKED_DOOR_7);
    constexpr EnumRangeInclusive<TerrainTag> jammed_door_tags(TerrainTag::JAMMED_DOOR_0, TerrainTag::JAMMED_DOOR_7);
    Door door;
    door.open = TerrainTag::OPEN_DOOR;
    door.broken = TerrainTag::BROKEN_DOOR;
    door.closed = TerrainTag::CLOSED_DOOR;
    door.locked.assign(locked_door_tags.begin(), locked_door_tags.end());
    door.jammed.assign(jammed_door_tags.begin(), jammed_door_tags.end());
    this->doors.emplace(DoorKind::DOOR, door);

    constexpr EnumRangeInclusive<TerrainTag> locked_glass_door_tags(TerrainTag::LOCKED_GLASS_DOOR_1, TerrainTag::LOCKED_GLASS_DOOR_7);
    constexpr EnumRangeInclusive<TerrainTag> jammed_glass_door_tags(TerrainTag::JAMMED_GLASS_DOOR_0, TerrainTag::JAMMED_GLASS_DOOR_7);
    Door glass_door;
    glass_door.open = TerrainTag::OPEN_GLASS_DOOR;
    glass_door.broken = TerrainTag::BROKEN_GLASS_DOOR;
    glass_door.closed = TerrainTag::CLOSED_GLASS_DOOR;
    glass_door.locked.assign(locked_glass_door_tags.begin(), locked_glass_door_tags.end());
    glass_door.jammed.assign(jammed_glass_door_tags.begin(), jammed_glass_door_tags.end());
    this->doors.emplace(DoorKind::GLASS_DOOR, glass_door);

    Door curtain;
    curtain.open = TerrainTag::OPEN_CURTAIN;
    curtain.broken = TerrainTag::OPEN_CURTAIN;
    curtain.closed = TerrainTag::CLOSED_CURTAIN;
    curtain.locked.push_back(TerrainTag::CLOSED_CURTAIN);
    curtain.jammed.push_back(TerrainTag::CLOSED_CURTAIN);
    this->doors.emplace(DoorKind::CURTAIN, curtain);
}

const Doors &Doors::get_instance()
{
    return instance;
}

const Door &Doors::get_door(DoorKind dk) const
{
    return this->doors.at(dk);
}

TerrainTag Doors::select_locked_tag(DoorKind dk) const
{
    const auto &door = this->doors.at(dk);
    return rand_choice(door.locked);
}

TerrainTag Doors::select_jammed_tag(DoorKind dk) const
{
    const auto &door = doors.at(dk);
    return rand_choice(door.jammed);
}
