#include "system/floor/town-records.h"
#include "system/angband-exceptions.h"
#include "system/floor/town-list.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

TownRecords TownRecords::instance{};

TownRecords &TownRecords::get_instance()
{
    return instance;
}

bool TownRecords::has_visited(TownId town_id) const
{
    this->validate_town_id(town_id);
    if (enum2i(town_id) == SECRET_TOWN) {
        return false;
    }

    return this->visited_ids.has(town_id);
}

void TownRecords::set_visited(TownId town_id)
{
    this->validate_town_id(town_id);
    if (enum2i(town_id) == SECRET_TOWN) {
        return;
    }

    this->visited_ids.set(town_id);
}

void TownRecords::initialize()
{
    this->visited_ids.clear();
    this->visited_ids.set(TownId::OUTPOST);
}

void TownRecords::set_ids(const EnumClassFlagGroup<TownId> &loaded_data)
{
    this->visited_ids = loaded_data;
}

EnumClassFlagGroup<TownId> TownRecords::get_ids() const
{
    return this->visited_ids;
}

void TownRecords::validate_town_id(TownId town_id) const
{
    if ((town_id < TownId::OUTPOST) || (town_id >= TownId::MAX)) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Town ID: {}", enum2i(town_id)));
    }
}
