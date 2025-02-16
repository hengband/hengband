#pragma once

class Direction;
class PlayerType;
bool wall_to_mud(PlayerType *player_ptr, const Direction &dir, int dam);
bool wizard_lock(PlayerType *player_ptr, const Direction &dir);
bool destroy_door(PlayerType *player_ptr, const Direction &dir);
bool disarm_trap(PlayerType *player_ptr, const Direction &dir);
