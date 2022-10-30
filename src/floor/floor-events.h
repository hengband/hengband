#pragma once

class PlayerType;
class FloorType;
void day_break(PlayerType *player_ptr);
void night_falls(PlayerType *player_ptr);
void update_dungeon_feeling(PlayerType *player_ptr);
void glow_deep_lava_and_bldg(PlayerType *player_ptr);
void forget_lite(FloorType *floor_ptr);
void forget_view(FloorType *floor_ptr);
