#pragma once

#include "system/angband.h"

// Quest Generator
typedef struct qg_type {
    char *buf;
    int ymin;
    int xmin;
    int ymax;
    int xmax;
    int *y;
    int *x;
} qg_type;

typedef errr (*process_dungeon_file_pf)(player_type *, concptr, int, int, int, int);

qg_type *initialize_quest_generator_type(qg_type *qg_ptr, char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
errr generate_fixed_map_floor(player_type *player_ptr, qg_type *qg_ptr, process_dungeon_file_pf parse_fixed_map);
