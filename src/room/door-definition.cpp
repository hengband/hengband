#include "room/door-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "term/z-rand.h"

Door::Door(TerrainTag open, TerrainTag broken, TerrainTag closd, const EnumRangeInclusive<TerrainTag> &locked, const EnumRangeInclusive<TerrainTag> &jammed)
    : open(open)
    , broken(broken)
    , closed(closd)
    , locked(locked)
    , jammed(jammed)
{
}

Doors Doors::instance{};

Doors::Doors()
{
    constexpr EnumRangeInclusive<TerrainTag> locked_door_tags(TerrainTag::LOCKED_DOOR_1, TerrainTag::LOCKED_DOOR_7);
    constexpr EnumRangeInclusive<TerrainTag> jammed_door_tags(TerrainTag::JAMMED_DOOR_0, TerrainTag::JAMMED_DOOR_7);
    Door door(TerrainTag::OPEN_DOOR, TerrainTag::BROKEN_DOOR, TerrainTag::CLOSED_DOOR, locked_door_tags, jammed_door_tags);
    this->doors.emplace(DoorKind::DOOR, door);

    constexpr EnumRangeInclusive<TerrainTag> locked_glass_door_tags(TerrainTag::LOCKED_GLASS_DOOR_1, TerrainTag::LOCKED_GLASS_DOOR_7);
    constexpr EnumRangeInclusive<TerrainTag> jammed_glass_door_tags(TerrainTag::JAMMED_GLASS_DOOR_0, TerrainTag::JAMMED_GLASS_DOOR_7);
    Door glass_door(TerrainTag::OPEN_GLASS_DOOR, TerrainTag::BROKEN_GLASS_DOOR, TerrainTag::CLOSED_GLASS_DOOR, locked_glass_door_tags, jammed_glass_door_tags);
    this->doors.emplace(DoorKind::GLASS_DOOR, glass_door);

    constexpr EnumRangeInclusive<TerrainTag> curtain_tags(TerrainTag::OPEN_CURTAIN, TerrainTag::OPEN_CURTAIN);
    Door curtain(TerrainTag::OPEN_CURTAIN, TerrainTag::OPEN_CURTAIN, TerrainTag::CLOSED_CURTAIN, curtain_tags, curtain_tags);
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
