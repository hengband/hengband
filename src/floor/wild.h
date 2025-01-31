#pragma once

#include "system/angband.h"
#include <vector>

enum parse_error_type : int;

/*
 * A structure describing a wilderness area with a terrain or a town
 */
enum class WildernessTerrain;
enum class DungeonId;
struct wilderness_type {
    WildernessTerrain terrain;
    int16_t town;
    int road;
    uint32_t seed;
    DEPTH level;
    DungeonId entrance;
};

extern std::vector<std::vector<wilderness_type>> wilderness;
extern bool reinit_wilderness;

class PlayerType;
void wilderness_gen(PlayerType *player_ptr);
void wilderness_gen_small(PlayerType *player_ptr);
void init_wilderness_terrains();
void init_wilderness_encounter();
void seed_wilderness();
parse_error_type parse_line_wilderness(PlayerType *player_ptr, char *buf, int xmin, int xmax, int *y, int *x);
bool change_wild_mode(PlayerType *player_ptr, bool encount);
