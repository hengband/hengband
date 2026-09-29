#include "info-reader/town-preferences-reader.h"
#include "system/dungeon/quest-fixed-map.h"
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

namespace {
bool is_valid_town_special(const nlohmann::json &value)
{
    if (!value.is_number_integer()) {
        return false;
    }

    constexpr auto minimum = std::numeric_limits<int16_t>::min();
    constexpr auto maximum = std::numeric_limits<int16_t>::max();
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() <= static_cast<uint64_t>(maximum);
    }

    const auto special = value.get<int64_t>();
    return special >= minimum && special <= maximum;
}
}

TownPreferencesReader::TownPreferencesReader(const nlohmann::json &data)
    : data(data)
{
}

parse_error_type TownPreferencesReader::read(TownPreferencesLegend &legend, TownPreferencesCellParser parse_cell) const
{
    if (!this->data.is_object() || !this->data.contains("version") || !this->data["version"].is_number_integer() || this->data["version"] != 1 ||
        !this->data.contains("legend") || !this->data["legend"].is_object() || this->data["legend"].empty()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    TownPreferencesLegend parsed;
    for (const auto &[symbol, cell_data] : this->data["legend"].items()) {
        if (symbol.size() != 1 || symbol.front() < '!' || symbol.front() > '~' || !cell_data.is_object() ||
            !cell_data.contains("terrain") || !cell_data["terrain"].is_string() || !cell_data.contains("caveInfo") || !cell_data["caveInfo"].is_array()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        for (const auto &flag : cell_data["caveInfo"]) {
            if (!flag.is_string()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
        }
        if (cell_data.contains("special") && !is_valid_town_special(cell_data["special"])) {
            return PARSE_ERROR_INVALID_VALUE;
        }

        QuestLegendCell cell;
        if (const auto err = parse_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
            return err;
        }
        parsed.emplace_back(static_cast<unsigned char>(symbol.front()), cell.grid);
    }

    legend = std::move(parsed);
    return PARSE_ERROR_NONE;
}
