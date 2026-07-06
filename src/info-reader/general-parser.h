#pragma once

#include "object-enchant/object-ego.h"
#include "system/angband.h"
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tuple>

enum parse_error_type : int;
enum class FixedArtifactId : short;
enum class TerrainTag;
struct dungeon_grid {
    FEAT_IDX feature; /* Terrain feature */
    MONSTER_IDX monster; /* Monster */
    OBJECT_IDX object; /* Object */
    EgoType ego; /* Ego-Item */
    FixedArtifactId artifact; /* Artifact */
    IDX trap; /* Trap */
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_NO_TELEPORT_DEST, CAVE_ROOM */
    int16_t special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */

    void set_terrain_id(TerrainTag tag);
    void set_trap_id(TerrainTag tag);
};

extern dungeon_grid letter[255];

enum class DefinitionHashDataType;
class FloorType;

using Parser = std::function<int(std::string_view)>;
std::tuple<int, int, std::string> init_info_txt(std::ifstream &ifs, DefinitionHashDataType dhdt, Parser parse_info_txt_line);
parse_error_type parse_line_feature(const FloorType &floor, std::string_view buf);
parse_error_type parse_line_building(std::string_view buf);
