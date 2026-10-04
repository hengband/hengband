#include "info-reader/town-preferences-reader.h"
#include "info-reader/json-reader-util.h"
#include "system/dungeon/quest-fixed-map.h"
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>

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
        int special = 0;
        if (cell_data.contains("special") &&
            info_set_integer(cell_data["special"], special, true, Range(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())) != PARSE_ERROR_NONE) {
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
