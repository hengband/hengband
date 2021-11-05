﻿#pragma once

#include "system/angband.h"
#include "info-reader/parse-error-types.h"

// Quest/Town/World Generator
typedef struct qtwg_type {
    char *buf;
    int ymin;
    int xmin;
    int ymax;
    int xmax;
    int *y;
    int *x;
} qtwg_type;

class PlayerType;
typedef parse_error_type (*process_dungeon_file_pf)(PlayerType *, concptr, int, int, int, int);

qtwg_type *initialize_quest_generator_type(qtwg_type *qg_ptr, char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qg_ptr, process_dungeon_file_pf parse_fixed_map);
