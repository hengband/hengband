#pragma once

class Direction;
class PlayerType;
int rod_effect(PlayerType *player_ptr, int sval, const Direction &dir, bool *use_charge, bool powerful);
void do_cmd_zap_rod(PlayerType *player_ptr);
