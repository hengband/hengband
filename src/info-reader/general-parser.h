#pragma once

#include "system/angband.h"

enum parse_error_type : int;

typedef struct dungeon_grid {
    FEAT_IDX feature; /* Terrain feature */
    MONSTER_IDX monster; /* Monster */
    OBJECT_IDX object; /* Object */
    EGO_IDX ego; /* Ego-Item */
    ARTIFACT_IDX artifact; /* Artifact */
    IDX trap; /* Trap */
    BIT_FLAGS cave_info; /* Flags for CAVE_MARK, CAVE_GLOW, CAVE_ICKY, CAVE_ROOM */
    s16b special; /* Reserved for special terrain info */
    int random; /* Number of the random effect */
} dungeon_grid;

extern dungeon_grid letter[255];

typedef struct angband_header angband_header;
typedef errr (*parse_info_txt_func)(char *buf, angband_header *head);
errr init_info_txt(FILE *fp, char *buf, angband_header *head, parse_info_txt_func parse_info_txt_line);
parse_error_type parse_line_feature(floor_type *floor_ptr, char *buf);
parse_error_type parse_line_building(char *buf);
