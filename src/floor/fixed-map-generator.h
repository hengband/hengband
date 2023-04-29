#pragma once

#include "system/angband.h"

// Quest/Town/World Generator
struct qtwg_type {
    char *buf;
    int ymin;
    int xmin;
    int ymax;
    int xmax;
    int *y;
    int *x;
};

class PlayerType;
enum parse_error_type : int;
typedef parse_error_type (*process_dungeon_file_pf)(PlayerType *, std::string_view, int, int, int, int);

qtwg_type *initialize_quest_generator_type(qtwg_type *qg_ptr, char *buf, int ymin, int xmin, int ymax, int xmax, int *y, int *x);
parse_error_type generate_fixed_map_floor(PlayerType *player_ptr, qtwg_type *qg_ptr, process_dungeon_file_pf parse_fixed_map);
