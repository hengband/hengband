#include "floor/floor-mode-changer.h"

FloorChangeModesStore FloorChangeModesStore::instance{};

FloorChangeModesStore &FloorChangeModesStore::get_instace()
{
    return instance;
}

EnumClassFlagGroup<FloorChangeMode> *FloorChangeModesStore::operator->()
{
    return &this->flag_change_modes;
}

const EnumClassFlagGroup<FloorChangeMode> *FloorChangeModesStore::operator->() const
{
    return &this->flag_change_modes;
}
