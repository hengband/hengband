#include "system/floor/town-records.h"
#include "system/angband-exceptions.h"
#include "system/floor/town-list.h"
#include "util/enum-converter.h"
#include <fmt/format.h>

TownRecords TownRecords::instance{};

/*!
 * @brief 町の ID を町の番号にする
 * @param town_id 町の ID
 * @return 町の番号 (TownList の添字。0 は町の外を表すので、辺境の地が 1 になる)
 * @exception std::out_of_range 町の ID が範囲外のとき
 */
size_t to_town_index(TownId town_id)
{
    if ((town_id < TownId::OUTPOST) || (town_id >= TownId::MAX)) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid Town ID: {}", enum2i(town_id)));
    }

    return static_cast<size_t>(town_id) + 1;
}

/*!
 * @brief 町の番号を町の ID にする
 * @param town_index 町の番号 (1 以上。0 は町の外を表すので、対応する ID は無い)
 * @return 町の ID
 * @exception std::out_of_range 町の番号に対応する ID が無いとき
 */
TownId to_town_id(size_t town_index)
{
    if ((town_index == 0) || (town_index > static_cast<size_t>(TownId::MAX))) {
        THROW_EXCEPTION(std::out_of_range, fmt::format("Invalid town index: {}", town_index));
    }

    return i2enum<TownId>(town_index - 1);
}

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

size_t TownRecords::size() const
{
    return enum2i(TownId::MAX);
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
