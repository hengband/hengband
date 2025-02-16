#pragma once

class Direction;
class PlayerType;
bool wand_effect(PlayerType *player_ptr, int sval, const Direction &dir, bool powerful, bool magic);
void do_cmd_aim_wand(PlayerType *player_ptr);
