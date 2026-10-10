#pragma once

#include <cstdint>

class PlayerType;
class FloorType;
void day_break(PlayerType *player_ptr);
void night_falls(PlayerType *player_ptr);
void update_dungeon_feeling(PlayerType *player_ptr);
void glow_deep_lava_and_bldg(PlayerType *player_ptr);
int64_t floor_rating_boost(int64_t delta);
