#pragma once

class PlayerType;
int rod_effect(PlayerType *player_ptr, int sval, int dir, bool *use_charge, bool powerful);
void do_cmd_zap_rod(PlayerType *player_ptr);
