#include "info-reader/town-definition-list-reader.h"
#include <nlohmann/json.hpp>
#include <string>

TownDefinitionListReader::TownDefinitionListReader(const nlohmann::json &data)
    : data(data)
{
}

parse_error_type TownDefinitionListReader::read(int town_index, TownMapMode mode, std::string &map_file) const
{
    if (!this->data.is_object() || !this->data.contains("version") || !this->data["version"].is_number_integer() || this->data["version"] != 1 ||
        !this->data.contains("towns") || !this->data["towns"].is_object() || this->data["towns"].empty()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    const auto &towns = this->data["towns"];
    const auto town = towns.find(std::to_string(town_index));
    if (town == towns.end()) {
        return PARSE_ERROR_NONE;
    }

    const nlohmann::json *selected = &*town;
    if (selected->is_object()) {
        const char *key = nullptr;
        switch (mode) {
        case TownMapMode::NORMAL:
            key = "normal";
            break;
        case TownMapMode::LITE:
            key = "lite";
            break;
        case TownMapMode::NONE:
            key = "none";
            break;
        default:
            return PARSE_ERROR_INVALID_VALUE;
        }
        const auto file = selected->find(key);
        if (file == selected->end()) {
            return PARSE_ERROR_INVALID_TYPE;
        }
        selected = &*file;
    }

    if (!selected->is_string()) {
        return PARSE_ERROR_INVALID_TYPE;
    }
    const auto &candidate = selected->get_ref<const std::string &>();
    if (!candidate.starts_with("towns/") || !candidate.ends_with(".jsonc") || candidate.find("..") != std::string::npos ||
        candidate.find('\\') != std::string::npos) {
        return PARSE_ERROR_INVALID_VALUE;
    }
    map_file = candidate;
    return PARSE_ERROR_NONE;
}
