#pragma once

#include "info-reader/parse-error-types.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include "util/point-2d.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

struct WildernessTownDefinition {
    int id = 0;
    std::string name;
    std::optional<std::string> alias;
};

struct WildernessLetterDefinition {
    char symbol = '\0';
    WildernessTerrain terrain;
    int level = 0;
    short town = 0;
    int road = 0;
};

struct WildernessMapDefinition {
    std::vector<WildernessLetterDefinition> letters;
    std::vector<std::string> layout;
    Pos2D starting_position = { 0, 0 };
};

struct WildernessDefinition {
    int width = 0;
    int height = 0;
    std::vector<WildernessTownDefinition> towns;
    WildernessMapDefinition normal;
    WildernessMapDefinition compact;
};

class WildernessReader {
public:
    explicit WildernessReader(const nlohmann::json &data);
    WildernessReader(nlohmann::json &&) = delete;
    WildernessReader(const WildernessReader &) = delete;
    WildernessReader(WildernessReader &&) = delete;
    WildernessReader &operator=(const WildernessReader &) = delete;
    WildernessReader &operator=(WildernessReader &&) = delete;

    int read(WildernessDefinition &definition) const;

private:
    int read_map(const nlohmann::json &data, int width, int height, WildernessMapDefinition &map) const;

    const nlohmann::json &data;
};

bool initialize_wilderness_definition();
parse_error_type apply_wilderness_definition();
