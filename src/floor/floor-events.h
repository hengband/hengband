﻿#pragma once

class PlayerType;
struct floor_type;
void day_break(PlayerType *player_ptr);
void night_falls(PlayerType *player_ptr);
void update_dungeon_feeling(PlayerType *player_ptr);
void glow_deep_lava_and_bldg(PlayerType *player_ptr);
void forget_lite(floor_type *floor_ptr);
void forget_view(floor_type *floor_ptr);
