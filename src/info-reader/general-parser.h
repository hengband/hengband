#pragma once

#include "system/angband.h"
#include <functional>
#include <string_view>

enum parse_error_type : int;

struct dungeon_grid {
    FEAT_IDX feature; /* Terrain feature */
    MONSTER_IDX monster; /* Monster */
    OBJECT_IDX object; /* Object */
    EGO_IDX ego; /* Ego-Item */
    ARTIFACT_IDX artifact; /* Artifact */
    IDX trap; /* Trap */
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
    int16_t special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */
};

extern dungeon_grid letter[255];

struct angband_header;
struct floor_type;

errr init_info_txt(FILE *fp, char *buf, angband_header *head, std::function<errr(std::string_view, angband_header *)> parse_info_txt_line);
parse_error_type parse_line_feature(floor_type *floor_ptr, char *buf);
parse_error_type parse_line_building(char *buf);
