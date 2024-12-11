#pragma once

#include "external-lib/include-json.h"
#include "object-enchant/object-ego.h"
#include "system/angband.h"
#include <functional>
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
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
    int16_t special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */

    void set_terrain_id(TerrainTag tag);
    void set_trap_id(TerrainTag tag);
};

extern dungeon_grid letter[255];

struct angband_header;
class FloorType;

using Parser = std::function<errr(std::string_view, angband_header *)>;
using JSONParser = std::function<errr(nlohmann::json &, angband_header *)>;
std::tuple<errr, int> init_info_txt(FILE *fp, char *buf, angband_header *head, Parser parse_info_txt_line);
parse_error_type parse_line_feature(FloorType *floor_ptr, char *buf);
parse_error_type parse_line_building(char *buf);
